#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

/* ===== 与 ESP32-S3 端保持一致 ===== */
#define WIFI_SSID   "ESP32S3_LINK"
#define WIFI_PASS   "12345678"
#define SERVER_IP   "192.168.4.1"
#define SERVER_PORT "8080"

void    ESP8266_Init(void);       /* USART1(PA9/PA10) 115200 + SysTick 1ms */
uint8_t ESP8266_Connect(void);    /* AT自检->连热点->建TCP, 成功返回1 */
uint8_t ESP8266_SendBtn(void);    /* 发送 "BTN\n", 成功返回1 */
uint8_t ESP8266_WaitLedAck(void); /* 等LED回执: 1=亮, 0=灭, 0xFF=超时 */
uint8_t ESP8266_IsClosed(void);   /* 检测连接是否被动断开 */

uint32_t GetTick(void);           /* 毫秒计数 */
void     Delay_ms(uint32_t ms);

#endif
