#include "stm32f10x.h"

void C(uint32_t t)
{
    uint32_t i, j;
    for(i = 1; i < t; i++)
        for(j = 0; j < 720; j++);
}

int main(void)
{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
		GPIO_InitTypeDef pin_a;
		pin_a.GPIO_Mode = GPIO_Mode_Out_PP;
		pin_a .GPIO_Pin = GPIO_Pin_All;
		pin_a .GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init (GPIOA,&pin_a );
	
    while(1)
    {
			GPIO_ResetBits(GPIOA,GPIO_Pin_0);
			C(10000);
			GPIO_SetBits(GPIOA,GPIO_Pin_0);
			C(10000);
    }
}
