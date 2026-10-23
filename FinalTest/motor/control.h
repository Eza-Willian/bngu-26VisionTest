// 视觉 ↔ 电控的固定接口。协议与报文约定见 motor/README.md。
#pragma once
#include <string>

namespace control {

// 一帧要发给电控的内容，由视觉侧填好。字段含义见 motor/README.md。
struct Frame {
    unsigned seq = 0;      // 帧序号，单调递增
    bool valid = false;    // 本帧结果是否有效；无效时坐标和角度都不许用
    float x = 0, y = 0;    // 目标中心（图像像素）
    double angle = 0;      // 角度（向上 0°、顺时针为正）
    bool hasAngle = false; // 本帧是否真的算出了角度；没算出时内部发 stop_value 哨兵
    std::string label;     // 类别标签，编码见 control.cpp 的 labelCode()
};

// 启动时调一次：读 config/serial.yml（设备、波特率、发送周期），开串口。
// 打不开不影响视觉流程，后面每次发送会自动重连。
bool open(const std::string& cfgPath);

// ── 固定接口 ──────────────────────────────────────────────
// 命令电机转向目标。errorDeg = 目标方向 − 箭头当前方向。
bool TurnAngle(Frame& frame, double errorDeg);

// 每帧状态（任务 10）：不需要转角时，也让电控知道本帧识别到了什么。
bool Send(const Frame& frame);
// ──────────────────────────────────────────────────────────

bool online();    // 串口是否在线，用于日志和现场排查
int  periodMs();  // 发送周期（ms），主循环用它控节拍

} // namespace control
