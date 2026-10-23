// 固定接口 control::TurnAngle() 的实现。分工与报文约定见 motor/README.md：
// 这里只做三件事——读配置、把 Frame 翻译成 angle_uart 的调用、记发送日志。
#include "control.h"
#include "angle_uart.h"
#include "paths.h"

#include <opencv2/core.hpp>   // 只为读 config/serial.yml（FileStorage）

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>

using namespace std;

namespace control {
namespace {

int g_periodMs = 30;
ofstream g_log;
angle_uart_t* g_u = nullptr;
bool g_openFailed = false;

double nowSec() {
    using namespace std::chrono;
    return duration<double>(system_clock::now().time_since_epoch()).count();
}

// 目标类别编码，和电控端约定好的一张表（只有 13 字节报文用得上）。
uint8_t labelCode(const string& s) {
    if (s == "red") return 1;
    if (s == "blue") return 2;
    if (s == "black") return 3;
    if (s == "green") return 4;
    if (s == "yellow") return 5;
    if (s == "circle") return 6;
    if (s == "rectangle") return 7;
    if (s == "triangle") return 8;
    if (s.size() == 1 && s[0] >= '0' && s[0] <= '9') return uint8_t(10 + s[0] - '0');
    if (s == "board") return 20;
    if (s == "board_incomplete") return 21;
    if (s == "arrow") return 22;
    return 0;
}

// sent_value = 真正写进串口的数；hasAngle=0 时是停车哨兵，别和 f.angle 混了。
void logFrame(const Frame& f, bool sent, float sentValue) {
    if (!g_log.is_open()) g_log.open(paths::pathOf("out/uart.log"), ios::app);
    if (!g_log) return;
    char buf[256];
    snprintf(buf, sizeof(buf),
             "%.3f seq=%u valid=%d x=%.1f y=%.1f angle=%.2f sent_value=%.2f label=%s sent=%d",
             nowSec(), f.seq, f.valid ? 1 : 0, f.x, f.y, f.angle, sentValue,
             f.label.c_str(), sent ? 1 : 0);
    g_log << buf << endl;
}

bool transmit(const Frame& f) {
    angle_uart_extra_t ex;
    ex.x = f.x;
    ex.y = f.y;
    ex.seq = f.seq;
    ex.label = labelCode(f.label);
    // hasAngle=0 时 angle_uart 内部发 stop_value 哨兵
    int n = angle_uart_send_ex(g_u, float(f.angle), f.hasAngle ? 1 : 0, &ex);
    bool ok = n > 0;
    logFrame(f, ok, f.hasAngle ? f.angle : angle_uart_stop_value(g_u));
    return ok;
}

} // namespace

bool open(const string& cfgPath) {
    string device = "/dev/ttyUSB0";
    int baud = 115200;
    string proto = "text";
    string jfFields = "angle,valid,x,y";
    string textFmt = "%.2f";
    double stopValue = 999.0;

    cv::FileStorage fs(cfgPath, cv::FileStorage::READ);
    if (fs.isOpened()) {
        if (!fs["device"].empty()) fs["device"] >> device;
        if (!fs["baud"].empty()) fs["baud"] >> baud;
        if (!fs["period_ms"].empty()) fs["period_ms"] >> g_periodMs;
        if (!fs["protocol"].empty()) fs["protocol"] >> proto;
        if (!fs["justfloat_fields"].empty()) fs["justfloat_fields"] >> jfFields;
        if (!fs["text_format"].empty()) fs["text_format"] >> textFmt;
        if (!fs["stop_value"].empty()) fs["stop_value"] >> stopValue;
    } else {
        cerr << "[串口] 读不到 " << cfgPath << "，用默认配置" << endl;
    }

    // 相对路径按工程根目录解析（.app 启动时工作目录是 "/"）
    if (!device.empty() && device[0] != '/' && device.rfind("/dev/", 0) != 0)
        device = paths::pathOf(device);

    g_u = angle_uart_open(device.c_str(), baud);
    if (!g_u) {
        cerr << "[串口] 创建句柄失败（内存不足？）" << endl;
        g_openFailed = true;
        return false;
    }

    // 协议名 -> 枚举；未知名字报出来退回 text
    if (proto == "text")              angle_uart_set_proto(g_u, ANGLE_UART_TEXT);
    else if (proto == "raw_float")    angle_uart_set_proto(g_u, ANGLE_UART_RAW_FLOAT);
    else if (proto == "vofa_justfloat") angle_uart_set_proto(g_u, ANGLE_UART_JUSTFLOAT);
    else if (proto == "vofa_firewater") angle_uart_set_proto(g_u, ANGLE_UART_FIREWATER);
    else if (proto == "raw13")        angle_uart_set_proto(g_u, ANGLE_UART_LINE13);
    else {
        cerr << "[串口] 不认识的 protocol: " << proto << "，退回 text" << endl;
        angle_uart_set_proto(g_u, ANGLE_UART_TEXT);
    }
    angle_uart_set_text_format(g_u, textFmt.c_str());
    angle_uart_set_justfloat_fields(g_u, jfFields.c_str());
    angle_uart_set_stop_value(g_u, float(stopValue));
    angle_uart_set_verbose(g_u, getenv("UART_DEBUG") ? 1 : 0);

    if (!angle_uart_is_open(g_u)) {
        // 打不开不影响视觉流程，后面每次发送会自动重连
        cerr << "[串口] " << device << " 打不开，后续发送会自动重试（不影响识别）" << endl;
        g_openFailed = true;
        return false;
    }
    cout << "[串口] 已打开 " << device << " @ " << baud << "，协议=" << proto
         << "，周期=" << g_periodMs << "ms" << endl;
    return true;
}

bool TurnAngle(Frame& frame, double errorDeg) {
    frame.angle = errorDeg;   // 固定接口只认这一个量
    frame.hasAngle = true;
    return transmit(frame);
}

bool Send(const Frame& frame) { return transmit(frame); }

bool online() { return g_u && angle_uart_is_open(g_u); }

int periodMs() { return g_periodMs; }

} // namespace control
