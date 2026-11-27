#include "key.h"
#include "esp8266.h"   /* Delay_ms */

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;    /* 内部上拉, 按下为低电平 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

uint8_t Key_Scan(void)
{
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
    {
        Delay_ms(20);                                          /* 按下消抖 */
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
        {
            while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0);  /* 等待释放 */
            Delay_ms(20);                                      /* 释放消抖 */
            return 1;
        }
    }
    return 0;
}
