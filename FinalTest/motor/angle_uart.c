/*
 * angle_uart.c —— angle_uart.h 的实现。纯 POSIX C，无第三方依赖。
 * 设计要点：串口只开一次一直复用；断开后每秒最多重连一次。
 */
#include "angle_uart.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

struct angle_uart {
    char device[256];
    int baud;
    int fd;
    int proto;
    int verbose;
    int reconnect;
    double last_try;      /* 上次尝试重连的时间，避免疯狂重试 */
    float stop_value;
    char text_format[16];
    int jf_fields[8];      /* JustFloat 字段序列，见 JF_* */
    int jf_count;
    int dtr;              /* -1=不动, 0=拉低, 1=拉高 */
    int rts;
    char last_error[160];
    unsigned seq;         /* LINE13 用的帧序号 */
};

/* JustFloat 可选的字段 */
enum { JF_ANGLE = 0, JF_VALID, JF_X, JF_Y, JF_SEQ, JF_LABEL };

/* ── 小工具 ───────────────────────────────────────────── */

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void set_error(angle_uart_t *h, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(h->last_error, sizeof(h->last_error), fmt, ap);
    va_end(ap);
    if (h->verbose) fprintf(stderr, "[angle_uart] %s\n", h->last_error);
}

#ifndef _WIN32
static speed_t speed_of(int baud) {
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    default: return B115200;
    }
}
#endif

/* ── 生命周期 ─────────────────────────────────────────── */

static int do_open(angle_uart_t *h) {
#ifdef _WIN32
    set_error(h, "Windows 暂不支持");
    return -1;
#else
    /* 必须 O_RDWR：O_RDONLY 打不开串口设备 */
    int fd = open(h->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        set_error(h, "打不开 %s: %s（设备名对吗？有权限吗？被别的程序占着吗？）",
                  h->device, strerror(errno));
        return -1;
    }
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        /* 不是 tty（普通文件/FIFO）：配不了参数也继续用，方便对着文件做测试转发 */
        if (h->verbose)
            fprintf(stderr, "[angle_uart] %s 不是 tty（%s），跳过串口参数配置\n",
                    h->device, strerror(errno));
    } else {
        /* 整体赋值清掉全部标志位（不用 cfmakeraw，它只清一部分，macOS 会留下 HUPCL 等） */
        tty.c_iflag = 0;
        tty.c_oflag = 0;
        tty.c_lflag = 0;
        tty.c_cflag = CS8 | CREAD | CLOCAL;   /* 8 数据位、开接收、忽略调制解调器线 */
        tty.c_cc[VMIN] = 0;                   /* 非阻塞读：不等最小字节数 */
        tty.c_cc[VTIME] = 0;                  /* 不等超时 */
        cfsetispeed(&tty, speed_of(h->baud));
        cfsetospeed(&tty, speed_of(h->baud));
        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            set_error(h, "设置串口参数失败: %s", strerror(errno));
            close(fd);
            return -1;
        }
        tcflush(fd, TCIOFLUSH);   /* 清掉打开时残留的数据 */
    }
    /* DTR/RTS 显式拉低（打开时默认高，会把部分板子摁在复位态） */
    if (h->dtr >= 0 || h->rts >= 0) {
        int bits = 0;
        if (h->dtr >= 0) bits |= TIOCM_DTR;
        if (h->rts >= 0) bits |= TIOCM_RTS;
        int on = 0;
        if (h->dtr == 1) on |= TIOCM_DTR;
        if (h->rts == 1) on |= TIOCM_RTS;
        if (on) ioctl(fd, TIOCMBIS, &on);            /* 置位 */
        int off = bits & ~on;
        if (off) ioctl(fd, TIOCMBIC, &off);          /* 清零（拉低） */
        if (h->verbose) fprintf(stderr, "[angle_uart] DTR/RTS 已设置\n");
    }

    h->fd = fd;
    h->last_error[0] = '\0';
    if (h->verbose) fprintf(stderr, "[angle_uart] 已打开 %s @ %d\n", h->device, h->baud);
    return 0;
#endif
}

angle_uart_t *angle_uart_open(const char *device, int baud) {
    if (!device || baud <= 0) return NULL;
    angle_uart_t *h = (angle_uart_t *)calloc(1, sizeof(*h));
    if (!h) return NULL;

    snprintf(h->device, sizeof(h->device), "%s", device);
    h->baud = baud;
    h->fd = -1;
    h->proto = ANGLE_UART_TEXT;
    h->verbose = 0;
    h->reconnect = 1;
    h->stop_value = 999.0f;
    h->dtr = 0;    /* 默认拉低 DTR/RTS */
    h->rts = 0;
    snprintf(h->text_format, sizeof(h->text_format), "%s", "%.0f");   /* 固件只认整数 */
    h->jf_fields[0] = JF_ANGLE; h->jf_fields[1] = JF_VALID;
    h->jf_fields[2] = JF_X;     h->jf_fields[3] = JF_Y;
    h->jf_count = 4;

    if (do_open(h) != 0) {
        /* 打不开也把句柄返回出去：允许调用方后面靠自动重连接上 */
        fprintf(stderr, "[angle_uart] 首次打开失败，后续发送会自动重试\n");
    }
    return h;
}

int angle_uart_send_once(const char *device, int baud, float angle_deg) {
    angle_uart_t *h = angle_uart_open(device, baud);
    if (!h) return -1;
    int n = angle_uart_send_angle(h, angle_deg, 1);
    angle_uart_close(h);
    return n;
}

int angle_uart_is_open(const angle_uart_t *h) { return h && h->fd >= 0; }

void angle_uart_close(angle_uart_t *h) {
    if (!h) return;
#ifndef _WIN32
    if (h->fd >= 0) close(h->fd);
#endif
    free(h);
}

/* ── 配置 ─────────────────────────────────────────────── */

int angle_uart_set_proto(angle_uart_t *h, angle_uart_proto_t proto) {
    if (!h || proto < 0 || proto > ANGLE_UART_LINE13) return -1;
    h->proto = proto;
    return 0;
}

int angle_uart_set_text_format(angle_uart_t *h, const char *fmt) {
    if (!h || !fmt || !*fmt) return -1;
    /* 只允许一个 %f 类转换，防止格式串写错把缓冲区搞爆 */
    if (!strstr(fmt, "%") || strchr(fmt, 's') || strchr(fmt, 'n')) return -1;
    snprintf(h->text_format, sizeof(h->text_format), "%s", fmt);
    return 0;
}

int angle_uart_set_justfloat_fields(angle_uart_t *h, const char *csv) {
    if (!h || !csv) return -1;
    int n = 0;
    const char *p = csv;
    while (*p && n < 8) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != ',' && *p != ' ') p++;
        size_t len = (size_t)(p - start);
        int f = -1;
        if (len == 5 && !strncmp(start, "angle", 5)) f = JF_ANGLE;
        else if (len == 5 && !strncmp(start, "valid", 5)) f = JF_VALID;
        else if (len == 1 && *start == 'x') f = JF_X;
        else if (len == 1 && *start == 'y') f = JF_Y;
        else if (len == 3 && !strncmp(start, "seq", 3)) f = JF_SEQ;
        else if (len == 5 && !strncmp(start, "label", 5)) f = JF_LABEL;
        else return -1;                 /* 不认识的字段名，别静默忽略 */
        h->jf_fields[n++] = f;
    }
    if (n == 0) return -1;
    h->jf_count = n;
    return 0;
}

void angle_uart_set_stop_value(angle_uart_t *h, float v) { if (h) h->stop_value = v; }
void angle_uart_set_reconnect(angle_uart_t *h, int on) { if (h) h->reconnect = on; }
void angle_uart_set_verbose(angle_uart_t *h, int on) { if (h) h->verbose = on; }

float angle_uart_stop_value(const angle_uart_t *h) { return h ? h->stop_value : 999.0f; }

/* DTR/RTS：-1=不碰，0=拉低，1=拉高。必须在 open 之前调用。 */
void angle_uart_set_dtr_rts(angle_uart_t *h, int dtr, int rts) {
    if (!h) return;
    h->dtr = dtr;
    h->rts = rts;
}
const char *angle_uart_last_error(const angle_uart_t *h) { return h ? h->last_error : "null"; }

/* ── 打包 ─────────────────────────────────────────────── */

static void put16(unsigned char *p, int v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

int angle_uart_pack(angle_uart_t *h, float angle_deg, int has_angle,
                    unsigned char *buf, int buf_size) {
    return angle_uart_pack_ex(h, angle_deg, has_angle, NULL, buf, buf_size);
}

int angle_uart_pack_ex(angle_uart_t *h, float angle_deg, int has_angle,
                       const angle_uart_extra_t *extra, unsigned char *buf, int buf_size) {
    if (!h || !buf || buf_size < 48) return -1;
    float v = has_angle ? angle_deg : h->stop_value;
    float ex = extra ? extra->x : 0.0f;
    float ey = extra ? extra->y : 0.0f;
    unsigned eseq = extra ? extra->seq : 0;
    unsigned char elabel = extra ? extra->label : 0;

    switch (h->proto) {
    case ANGLE_UART_TEXT: {
        char tmp[64];
        int n = snprintf(tmp, sizeof(tmp), h->text_format, (double)v);
        if (n < 0 || n > (int)sizeof(tmp) - 3) return -1;
        tmp[n++] = '\r';                   /* 电控按回车断帧 */
        tmp[n++] = '\n';
        memcpy(buf, tmp, (size_t)n);
        return n;
    }
    case ANGLE_UART_RAW_FLOAT:
        memcpy(buf, &v, sizeof(float));     /* 小端机器上就是小端字节序 */
        return (int)sizeof(float);
    case ANGLE_UART_JUSTFLOAT: {
        float vals[8];
        int n = 0;
        for (int i = 0; i < h->jf_count && n < 8; i++) {
            switch (h->jf_fields[i]) {
            case JF_ANGLE: vals[n++] = v; break;
            case JF_VALID: vals[n++] = has_angle ? 1.0f : 0.0f; break;
            case JF_X:     vals[n++] = ex; break;
            case JF_Y:     vals[n++] = ey; break;
            case JF_SEQ:   vals[n++] = (float)eseq; break;
            case JF_LABEL: vals[n++] = (float)elabel; break;
            default: break;
            }
        }
        if (n == 0) return -1;
        memcpy(buf, vals, sizeof(float) * (size_t)n);
        buf[n * 4 + 0] = 0x00; buf[n * 4 + 1] = 0x00;   /* +inf 帧尾 */
        buf[n * 4 + 2] = 0x80; buf[n * 4 + 3] = 0x7F;
        return n * 4 + 4;
    }
    case ANGLE_UART_FIREWATER: {
        char tmp[128];
        int n = snprintf(tmp, sizeof(tmp), "angle:%.3f,valid:%d,x:%.1f,y:%.1f\r\n",
                         (double)v, has_angle ? 1 : 0, (double)ex, (double)ey);
        if (n < 0 || n > (int)sizeof(tmp)) return -1;
        memcpy(buf, tmp, (size_t)n);
        return n;
    }
    case ANGLE_UART_LINE13: {
        unsigned char *b = buf;
        b[0] = 0xAA; b[1] = 0x55;
        put16(b + 2, (int)(extra ? eseq : h->seq++) & 0xFFFF);
        b[4] = has_angle ? 1 : 0;
        put16(b + 5, (int)(ex < 0 ? ex - 0.5f : ex + 0.5f));   /* x，四舍五入 */
        put16(b + 7, (int)(ey < 0 ? ey - 0.5f : ey + 0.5f));   /* y */
        put16(b + 9, (int)(v * 10.0f + (v < 0 ? -0.5f : 0.5f)));  /* 0.1 度为单位 */
        b[11] = elabel;
        b[12] = 0;
        for (int i = 2; i <= 11; i++) b[12] ^= b[i];
        return 13;
    }
    default:
        return -1;
    }
}

/* ── 发送 ─────────────────────────────────────────────── */

int angle_uart_send_angle(angle_uart_t *h, float angle_deg, int has_angle) {
    return angle_uart_send_ex(h, angle_deg, has_angle, NULL);
}

int angle_uart_send_ex(angle_uart_t *h, float angle_deg, int has_angle,
                       const angle_uart_extra_t *extra) {
    if (!h) return -1;
    unsigned char buf[192];
    int n = angle_uart_pack_ex(h, angle_deg, has_angle, extra, buf, (int)sizeof(buf));
    if (n < 0) { set_error(h, "打包失败（协议或格式串有问题）"); return -1; }

#ifdef _WIN32
    return -1;
#else
    if (h->fd < 0) {
        if (!h->reconnect) { set_error(h, "串口未打开"); return -1; }
        double t = now_sec();
        if (t - h->last_try < 1.0) return -1;   /* 每秒最多重连一次 */
        h->last_try = t;
        if (do_open(h) != 0) return -1;
        fprintf(stderr, "[angle_uart] 已重连 %s\n", h->device);
    }

    ssize_t w = write(h->fd, buf, (size_t)n);
    if (w != n) {
        set_error(h, "写串口失败: %s（设备被拔了？）", strerror(errno));
        close(h->fd);                            /* 标记断开，下次自动重连 */
        h->fd = -1;
        return -1;
    }

    if (h->verbose) {
        fprintf(stderr, "[angle_uart] 发出 %d 字节:", n);
        for (int i = 0; i < n; i++) fprintf(stderr, " %02X", buf[i]);
        fprintf(stderr, "   (%s)\n", has_angle ? "角度" : "停车哨兵");
    }
    return n;
#endif
}
