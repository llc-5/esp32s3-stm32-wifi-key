# ESP32-S3 ⇄ STM32 无线按键控灯

STM32 按键 → USART1 AT 指令 → ESP8266（出厂 AT 固件，**无需烧录程序**）→ WiFi/TCP → ESP32-S3（热点 + TCP 服务器）→ LED 翻转 + 串口显示。

## 通信参数

| 参数 | 值 |
|---|---|
| 热点 SSID | ESP32S3_LINK |
| 热点密码 | 12345678 |
| 服务器 IP | 192.168.4.1（ESP32 热点默认网关） |
| TCP 端口 | 8080 |
| 按键消息 | `BTN\n`（4 字节，每收到一次 LED 翻转一次） |
| 串口波特率 | STM32↔ESP8266：115200；ESP32 调试串口：115200 |

## 目录结构

```
esp32s3-stm32-wifi-key/
├── README.md
└── ESP32S3_AP_LED/
    └── ESP32S3_AP_LED.ino    # Arduino IDE 直接打开烧录
```

STM32 代码已移入 Keil 工程：`E:\STM32学习\STM32-ESP32无网连接\User`
（main.c、esp8266.c/h、key.c/h、led.c/h；原流水灯 main.c 备份为 main.c.bak）。
**后续 STM32 端的全部更改都在该 User 目录进行。**

## 接线表

### ESP8266（ESP-01 / ESP-01S）↔ STM32F103C8T6

| ESP8266 引脚 | 接到 | 说明 |
|---|---|---|
| VCC | 独立 3.3V 电源（≥500mA，如 AMS1117-3.3） | 勿只靠 STM32 板载 3.3V，发射瞬间电流可达 300~400mA |
| GND | 与 STM32、3.3V 电源共地 | 必须共地 |
| EN / CH_PD | 3.3V | ESP-01 必须接；ESP-01S 板内已上拉，建议仍接 |
| TX | STM32 PA10（USART1_RX） | 交叉接 |
| RX | STM32 PA9（USART1_TX） | 双方均 3.3V 电平，可直连 |
| IO0 / IO2 / RST | 悬空 | 正常运行模式，IO0 拉低会进烧录模式 |

### 按键（STM32 侧）

| 元件 | 接法 |
|---|---|
| 轻触按键 | 一脚 PB0，另一脚 GND（内部上拉，按下为低） |
| LED（STM32 侧） | PA0，默认低电平点亮：LED 正极接 3.3V，负极经 220Ω 接 PA0；若为高电平点亮接法，把 led.h 的 LED_ACTIVE_LOW 改为 0 |

### LED（ESP32-S3 侧）

| 元件 | 接法 |
|---|---|
| 普通 LED | GPIO2 → 220Ω 电阻 → LED 正极，LED 负极 → GND |

注：很多 S3 开发板的板载灯是 WS2812 彩灯（GPIO48/38），digitalWrite 点不亮，所以用外接 LED。

## 使用步骤

1. **烧 ESP32-S3**：Arduino IDE 安装 esp32 板包，开发板选 `ESP32S3 Dev Module`；若串口走原生 USB 口，`工具 → USB CDC On Boot → Enabled`。烧录后开串口监视器（115200），应显示"热点已开启"。
2. **按接线表接线**，确认 ESP8266 供电充足、共地。
3. **Keil5**：代码已在工程 User 目录。在 Keil 里右键 User 分组 → Add Existing Files，把 `esp8266.c`、`key.c` 加进来（main.c 已在工程里）；`stm32f10x_it.c` 的空 `SysTick_Handler` 已注释掉。若链接报 USART/NVIC 函数未定义，把 Library 里的 `stm32f10x_usart.c`、`misc.c` 也加入工程。编译烧录。
4. **上电顺序**：先 ESP32-S3，后 STM32。ESP32 串口应打印 `已连接`。
5. **按 PB0**：串口打印 `收到信息: BTN` + `LED 点亮`；再按一次 → `LED 熄灭`。STM32 侧 PA0 LED 依据 ESP32 回执（LED:ON/OFF）同步亮灭。

## 预期串口输出（ESP32 端）

```
热点已开启: ESP32S3_LINK
密码: 12345678
IP地址: 192.168.4.1
TCP服务器已启动, 等待STM32连接...
已连接
客户端IP: 192.168.4.2
收到信息: BTN
LED 点亮
收到信息: BTN
LED 熄灭
```

空闲时每 5 秒打印一次"等待STM32连接..."心跳；断开时打印"已断开"。

## 故障排查

| 现象 | 原因 | 处理 |
|---|---|---|
| AT 自检一直失败（连不上热点） | 供电不足 / TX-RX 没交叉 / EN 没拉高 / 波特率不符 | 换独立 3.3V；核对 PA9→RX、PA10→TX；老固件波特率 9600，改 esp8266.c 中一处 |
| AT 有响应但 CWJAP 失败 | ESP32 未上电 / SSID 密码不一致 | 先开 ESP32；两端参数核对 |
| 连接后立刻 CLOSED | ESP32 重启过 / 端口不对 | 保持两端参数一致，STM32 会自动重连 |
| Keil 中文注释乱码 | 编辑器编码 | Edit → Configuration → Editor → Encoding 选 UTF-8 |
| 确认接线供电都对仍无 AT 响应 | 模块可能没有 AT 固件 | **需要刷 AT 固件，先与助手确认后再进行** |

## Git 建议

每完成一个阶段（烧录验证通过、改参数、加功能）提交一次：

```
git add .
git commit -m "feat: ESP32S3热点 + STM32/ESP8266 按键无线控灯"
```

推送远程前须先经本人确认。