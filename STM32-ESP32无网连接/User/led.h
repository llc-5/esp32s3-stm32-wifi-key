#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/* LED 接 PA0 (原流水灯实验引脚)
 * 默认低电平点亮: LED正极接3.3V, 负极经220Ω电阻接PA0
 * 若你的接法是 PA0->电阻->LED正极->GND (高电平点亮), 把下面宏改为 0 */
#define LED_ACTIVE_LOW 1

void LED_Init(void);
void LED_Set(uint8_t on);   /* 1=亮, 0=灭 */

#endif
