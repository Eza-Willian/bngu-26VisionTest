/*
 * angle_uart.h —— 把"角度"通过串口发出去，一个 .h + 一个 .c，无第三方依赖。
 * 用法、协议字节格式、现场注意事项都在 motor/README.md。
 */
#ifndef ANGLE_UART_H
#define ANGLE_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* 发送协议，先问清电控按哪种解析再选。默认 TEXT（现场实测有效）。 */
typedef enum {
    ANGLE_UART_TEXT = 0,   /* "30\r\n" 文本，读到回车解析 */
    ANGLE_UART_RAW_FLOAT,  /* 4 字节 float32 小端，无帧尾 */
    ANGLE_UART_JUSTFLOAT,  /* float32 + 00 00 80 7F（VOFA+ JustFloat） */
    ANGLE_UART_FIREWATER,  /* "angle:-84.8,valid:1,...\r\n"（VOFA+ FireWater） */
    ANGLE_UART_LINE13      /* 13 字节自定义报文，格式见本文件末尾 */
} angle_uart_proto_t;

/* 附带信息，只有 LINE13 用得上（其余协议只发角度）。不用就传 NULL。 */
typedef struct {
    float x;               /* 目标中心 x（像素） */
    float y;               /* 目标中心 y（像素） */
    unsigned seq;          /* 帧序号，单调递增 */
    unsigned char label;   /* 类别编码，和电控约定的表 */
} angle_uart_extra_t;

/* 不透明句柄。 */
typedef struct angle_uart angle_uart_t;

/*
 * 打开串口。成功返回句柄，失败返回 NULL。
 * device: Linux 用 /dev/ttyUSB0；macOS 用 /dev/cu.usbserial-*（不要用 /dev/tty.usbserial-*）
 * baud:   两边必须一致，常用 115200
 */
angle_uart_t *angle_uart_open(const char *device, int baud);

/* 一行发角度：内部自己 open -> send -> close。仅适合临时测试/单次动作；
 * 高频循环里请自己 open 一次、反复调 send_angle（反复 open 会反复拉 DTR/RTS）。 */
int angle_uart_send_once(const char *device, int baud, float angle_deg);

/*
 * DTR/RTS 控制，默认已经都拉低（不拉低部分板子不响应）。
 * dtr/rts: -1=不碰，0=拉低，1=拉高。必须在 open 之前调用。
 */
void angle_uart_set_dtr_rts(angle_uart_t *h, int dtr, int rts);

/* 停车哨兵的值（has_angle=0 时实际发出去的就是它）。 */
float angle_uart_stop_value(const angle_uart_t *h);

/* 串口当前是否已打开（断开时为 0，会随发送自动重连）。 */
int angle_uart_is_open(const angle_uart_t *h);

/* 关闭并释放。传 NULL 安全。 */
void angle_uart_close(angle_uart_t *h);

/* 切换协议，默认 TEXT。返回 0 成功，-1 参数非法。 */
int angle_uart_set_proto(angle_uart_t *h, angle_uart_proto_t proto);

/* 文本协议的格式串，默认 "%.0f"（现场实测固件只认整数，带小数不动）。 */
int angle_uart_set_text_format(angle_uart_t *h, const char *fmt);

/* JustFloat 发哪几个量、什么顺序，默认 "angle,valid,x,y"。
 * 个数必须和电控接收端完全一致。可用字段：angle / valid / x / y / seq / label */
int angle_uart_set_justfloat_fields(angle_uart_t *h, const char *csv);

/* 目标丢失时发的哨兵值，默认 999.0（电控必须识别它停车）。 */
void angle_uart_set_stop_value(angle_uart_t *h, float v);

/* 串口断开后是否自动重连，默认开。 */
void angle_uart_set_reconnect(angle_uart_t *h, int on);

/* 打印每次发送的内容，默认关。 */
void angle_uart_set_verbose(angle_uart_t *h, int on);

/* 发一个角度。angle_deg 向上 0°、顺时针为正；has_angle=0 表示目标丢失，改发停车哨兵。
 * 返回实际写出的字节数，<0 失败。 */
int angle_uart_send_angle(angle_uart_t *h, float angle_deg, int has_angle);

/* 同上，带上 x/y/seq/label（只对 LINE13 有意义）。extra 传 NULL 等价于各字段为 0。 */
int angle_uart_send_ex(angle_uart_t *h, float angle_deg, int has_angle,
                       const angle_uart_extra_t *extra);

/* 打包成字节（不发送），方便自己 write 或做单元测试。buf 至少 32 字节。 */
int angle_uart_pack(angle_uart_t *h, float angle_deg, int has_angle,
                    unsigned char *buf, int buf_size);
int angle_uart_pack_ex(angle_uart_t *h, float angle_deg, int has_angle,
                       const angle_uart_extra_t *extra, unsigned char *buf, int buf_size);

/* 最后一次失败的原因，NULL 表示没失败过。 */
const char *angle_uart_last_error(const angle_uart_t *h);

#ifdef __cplusplus
}
#endif

/*
 * 13 字节报文格式（ANGLE_UART_LINE13，小端）：
 *   0  1  | 2  3 | 4     | 5  6 | 7  8 | 9  10  | 11    | 12
 *   AA 55 | seq  | valid | x    | y    | angle  | label | xor
 *   angle 单位 0.1 度（有符号）；xor = 字节 2~11 逐字节异或。
 */
#endif /* ANGLE_UART_H */
