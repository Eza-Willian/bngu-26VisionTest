# motor/ —— 视觉 ↔ 电控 的固定接口（考核人直接用它发角度）

这个文件夹是**独立的电控模块**：把整个 `motor/` 拷到任何工程里都能用。
视觉侧不用关心串口怎么开、报文怎么拼、断了怎么重连——只调用接口函数即可。

## 文件清单

| 文件 | 作用 | 依赖 |
|---|---|---|
| `control.h` | **固定接口** `TurnAngle` / `Send` 的声明 | 无 |
| `control.cpp` | 接口实现：读 `config/serial.yml`、开串口、记日志 | OpenCV（只用来读 YAML 配置） |
| `angle_uart.h` / `angle_uart.c` | 串口打开、报文打包、发送（底层） | **零依赖，纯 POSIX C** |
| `paths.h` | 工程根目录定位（header-only） | 无 |

## 固定接口（考核人只关心这个）

```cpp
#include "control.h"

// 启动时调一次：读配置（设备、波特率、发送周期），打开串口。
// 打不开不影响流程，之后每次发送会自动重连。
bool control::open(const std::string& cfgPath);   // cfgPath: config/serial.yml

// ── 唯一需要反复调用的两个函数 ────────────────────────────
// 命令电机转向：errorDeg = 目标方向 − 箭头当前方向。
// 角度约定：向上 0°、顺时针为正；电控按符号决定正反转。
bool control::TurnAngle(control::Frame& frame, double errorDeg);

// 每帧状态（不需要转角时用，让电控知道识别到了什么）。
bool control::Send(const control::Frame& frame);
// ────────────────────────────────────────────────────────

bool control::online();   // 串口是否在线（排查用）
int  control::periodMs(); // 发送周期（ms），主循环用它控节拍
```

`Frame` 里要填的字段：

```cpp
struct Frame {
    unsigned seq;      // 帧序号，单调递增
    bool valid;        // 本帧结果是否有效
    float x, y;        // 目标中心（图像像素）
    double angle;      // 角度
    bool hasAngle;     // 本帧是否真的算出了角度（没算出时不能当 0 发！）
    std::string label; // 标签，编码见 control.cpp 的 labelCode()
};
```

## 最小用法

```cpp
// 一个最小例子（另存为 your_main.cpp 即可）
#include "control.h"
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    control::open(argc > 1 ? argv[1] : "config/serial.yml");

    control::Frame f;
    f.label = "red_circle_3";
    f.valid = true;
    f.hasAngle = true;

    for (unsigned i = 0; i < 100; i++) {
        f.seq = i;
        f.angle = -25;                     // 目标方向 25°，电控正=左，所以取反
        control::TurnAngle(f, f.angle);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(control::periodMs()));
    }
    return 0;
}
```

编译（需要 OpenCV）：

```bash
c++ -std=c++17 your_main.cpp control.cpp angle_uart.c -I. \
    $(pkg-config --cflags --libs opencv4) -o your_tool
```

## 配置 `config/serial.yml`（示例，现场核对过的值）

```yaml
device: "/dev/cu.usbserial-11320"   # Linux 上通常是 /dev/ttyUSB0 或 /dev/ttyACM0
baud: 115200
period_ms: 30
protocol: "text"        # text | raw_float | vofa_justfloat | vofa_firewater | raw13
text_format: "%.0f"     # 现场实测：固件只认整数，带小数不动！
stop_value: 999.0       # hasAngle=0 时发这个值，电控必须识别它停车
justfloat_fields: "angle"
```

## 报文约定（和电控对好的，改之前先问电控）

| 项 | 约定 |
|---|---|
| 文本协议格式 | `整数\r\n`，例如 `-25\r\n` |
| **整数** | 现场实测：`40.00` 发出去电机不动，`40` 才动。所以 `text_format` 必须是 `"%.0f"` |
| 角度范围 | ±180，有符号 |
| **正 = 左** | 发 −25 箭头到右边 25°，发 +40 到左边 40° |
| 没算出角度 | 发 `stop_value`（999），**绝不要把 0 当"没角度"发**——0 是合法位置（正上方） |
| **DTR/RTS** | 必须拉低。打开串口时它们默认是高，会把部分板子摁在复位态（现场踩过：不拉低电机完全不响应） |
| 重连 | 发送失败自动关句柄，之后每秒重试一次 |

## 底层 `angle_uart`（不用 OpenCV 时直接用这一层）

```c
#include "angle_uart.h"

angle_uart_t* u = angle_uart_open("/dev/cu.usbserial-11320", 115200);
angle_uart_set_proto(u, ANGLE_UART_TEXT);
angle_uart_set_text_format(u, "%.0f");        // 整数！
angle_uart_set_dtr_rts(u, 0, 0);              // 拉低，必须在 open 之前设好
angle_uart_send_angle(u, -25, 1);             // 发 -25 度
angle_uart_close(u);
```

完整函数表见 `angle_uart.h`，其余协议（raw_float / justfloat / firewater / 13字节）
用 `angle_uart_set_proto` 切换，打包可用 `angle_uart_pack_ex` 先看字节再发。

纯 C 零依赖，单独编译：

```bash
cc -std=c11 your_code.c angle_uart.c -o your_tool
```

## 自检（在主工程里）

```bash
./tools/verify_uart_pack.sh
```

打包字节回归（文本/二进制/停车哨兵，8 项），期望值和现场硬件核对过，
**全绿才说明这层没被改坏**。

## 现场总结的三个坑（别重踩）

1. **带小数点不动**：固件只认整数，格式一律 `%.0f`。
2. **DTR/RTS 不拉低不响应**：打开串口后必须显式拉低。
3. **正 = 左**：发正数箭头往左转，视觉侧算的角度要取反再发。
