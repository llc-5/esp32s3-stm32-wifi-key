/*
 * STM32F103C8T6 + ESP8266(AT固件) 无线按键控制 ESP32-S3 LED
 * 标准库 SPL / Keil5
 *
 * 功能:
 *   上电自动连 ESP32-S3 热点并建立 TCP (失败自动重试)
 *   PB0 按键按一次 -> 发送 "BTN" -> ESP32 端 LED 翻转
 *   PA0 本地 LED 根据 ESP32 回执同步亮灭
 *   掉线自动重连
 */
#include "stm32f10x.h"
#include "esp8266.h"
#include "key.h"
#include "led.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    ESP8266_Init();
    Key_Init();
    LED_Init();

    /* 上电先连热点 + 建TCP, 失败每秒重试 */
    while (ESP8266_Connect() == 0)
    {
        Delay_ms(1000);
    }

    while (1)
    {
        if (Key_Scan())
        {
            uint8_t ok = ESP8266_SendBtn();
            if (!ok)                           /* 发送失败: 重连后补发一次 */
            {
                while (ESP8266_Connect() == 0) Delay_ms(1000);
                ok = ESP8266_SendBtn();
            }
            if (ok)                            /* 按ESP32回执设置本地LED, 两边状态一致 */
            {
                uint8_t st = ESP8266_WaitLedAck();
                if (st != 0xFF) LED_Set(st);
            }
        }

        if (ESP8266_IsClosed())                /* 被动掉线: 自动重连 */
        {
            while (ESP8266_Connect() == 0) Delay_ms(1000);
        }
    }
}
