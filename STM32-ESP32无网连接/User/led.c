#include "led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    LED_Set(0);   /* 上电默认熄灭, 与ESP32端初始状态一致 */
}

void LED_Set(uint8_t on)
{
#if LED_ACTIVE_LOW
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, on ? Bit_SET : Bit_RESET);
#else
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, on ? Bit_RESET : Bit_SET);
#endif
}
