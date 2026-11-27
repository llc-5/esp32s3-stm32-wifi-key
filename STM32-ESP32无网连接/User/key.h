#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

void    Key_Init(void);   /* PB0 上拉输入, 按键另一端接GND */
uint8_t Key_Scan(void);   /* 检测一次有效按下(消抖+等待释放), 按下返回1 */

#endif
