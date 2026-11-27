/*
 * ESP32-S3: WiFi 热点 + TCP 服务器 + 按键 LED 翻转
 * 开发板: ESP32S3 Dev Module (Arduino IDE)
 * 串口监视器: 115200
 * 若用原生 USB 口看串口, 需在 工具 -> USB CDC On Boot 选 Enabled
 */
#include <WiFi.h>

/* ========== 可按需修改的参数 ========== */
const char *AP_SSID = "ESP32S3_LINK";   // 热点名称
const char *AP_PASS = "12345678";       // 热点密码(至少8位)
const uint16_t TCP_PORT = 8080;         // TCP 端口
const int LED_PIN = 2;                  // 外接LED: GPIO2 -> 220Ω电阻 -> LED正极, LED负极 -> GND
/* ===================================== */

WiFiServer server(TCP_PORT);
WiFiClient client;
bool ledOn = false;
String rxLine = "";
uint32_t lastWaitPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.println();
  Serial.print("热点已开启: ");
  Serial.println(AP_SSID);
  Serial.print("密码: ");
  Serial.println(AP_PASS);
  Serial.print("IP地址: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
  server.setNoDelay(true);
  Serial.println("TCP服务器已启动, 等待STM32连接...");
}

void loop() {
  /* 接受新连接: hasClient()探测 + available()取出, 兼容 2.x / 3.x 核心 */
  if (server.hasClient()) {
    WiFiClient newClient = server.available();
    if (newClient) {
      if (client) {                          // 只保留一个客户端
        client.stop();
        Serial.println("旧连接已断开");
      }
      client = newClient;
      rxLine = "";
      Serial.println("已连接");
      Serial.print("客户端IP: ");
      Serial.println(client.remoteIP());
    }
  }

  /* 检测客户端断开 */
  if (client && !client.connected()) {
    client.stop();
    client = WiFiClient();
    rxLine = "";
    Serial.println("已断开");
  }

  /* 空闲时每5秒打印一次等待状态, 便于确认服务器存活 */
  if (!client && millis() - lastWaitPrint >= 5000) {
    lastWaitPrint = millis();
    Serial.println("等待STM32连接...");
  }

  /* 按行接收数据 */
  while (client && client.available()) {
    char c = (char)client.read();
    if (c == '\n') {
      rxLine.trim();
      if (rxLine.length() > 0) {
        Serial.print("收到信息: ");
        Serial.println(rxLine);
        if (rxLine == "BTN") {
          ledOn = !ledOn;                       // 每收到一次 BTN 翻转一次
          digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
          Serial.println(ledOn ? "LED 点亮" : "LED 熄灭");
          client.println(ledOn ? "LED:ON" : "LED:OFF");  // 回执: STM32本地LED据此同步
        }
      }
      rxLine = "";
    } else if (c != '\r') {
      rxLine += c;
      if (rxLine.length() > 64) rxLine = "";    // 防异常数据撑爆
    }
  }
}
