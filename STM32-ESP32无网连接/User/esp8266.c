/*
 * ESP8266 AT 指令驱动 (标准库 SPL)
 * USART1: PA9=TX -> ESP8266 RX, PA10=RX <- ESP8266 TX, 115200 8N1
 * ESP8266 使用出厂 AT 固件, 无需烧录任何程序
 *
 * 注意: 本文件包含 SysTick_Handler 和 USART1_IRQHandler,
 * 若工程的 stm32f10x_it.c 里已有同名函数, 请删除/注释那边的空函数, 否则重复定义报错
 */
#include "esp8266.h"
#include <string.h>

#define ESP_RX_BUF_SIZE 512

static char RxBuf[ESP_RX_BUF_SIZE];
static volatile uint16_t RxLen = 0;
static volatile uint32_t msTicks = 0;

/* ---------- 毫秒节拍 ---------- */
void SysTick_Handler(void)
{
    msTicks++;
}

uint32_t GetTick(void)
{
    return msTicks;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = GetTick();
    while (GetTick() - start < ms);
}

/* ---------- 串口底层 ---------- */
static void UART1_SendString(const char *s)
{
    while (*s)
    {
        USART_SendData(USART1, (uint8_t)*s++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

static void Rx_Clear(void)
{
    RxLen = 0;
    RxBuf[0] = '\0';
}

/* 等待关键字出现; 超时或出现 ERROR/FAIL 提前返回0 */
static uint8_t Wait_Ack(const char *ack, uint32_t timeout_ms)
{
    uint32_t start = GetTick();
    while (GetTick() - start < timeout_ms)
    {
        if (strstr(RxBuf, ack))     return 1;
        if (strstr(RxBuf, "ERROR")) return 0;
        if (strstr(RxBuf, "FAIL"))  return 0;
    }
    return 0;
}

static uint8_t SendCmd(const char *cmd, const char *ack, uint32_t timeout_ms)
{
    Rx_Clear();
    UART1_SendString(cmd);
    UART1_SendString("\r\n");
    return Wait_Ack(ack, timeout_ms);
}

/* ---------- 初始化 ---------- */
void ESP8266_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    SysTick_Config(SystemCoreClock / 1000);   /* 1ms 节拍 */

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* PA9 = USART1_TX -> ESP8266 RX */
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 = USART1_RX <- ESP8266 TX */
    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate            = 115200;  /* 老款AT固件若默认9600, 只改这里 */
    USART_InitStruct.USART_WordLength          = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits            = USART_StopBits_1;
    USART_InitStruct.USART_Parity              = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStruct);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

/* ---------- 接收中断 ---------- */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t d = (uint8_t)USART_ReceiveData(USART1);
        if (RxLen < ESP_RX_BUF_SIZE - 1)
        {
            RxBuf[RxLen++] = (char)d;
            RxBuf[RxLen]   = '\0';
        }
        else                        /* 缓冲满则丢弃重来 */
        {
            RxLen = 0;
            RxBuf[0] = '\0';
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/* ---------- 连热点 + 建TCP ---------- */
uint8_t ESP8266_Connect(void)
{
    uint8_t i, alive = 0;

    for (i = 0; i < 5; i++)                     /* AT 自检, 最多试5次 */
    {
        if (SendCmd("AT", "OK", 500)) { alive = 1; break; }
        Delay_ms(200);
    }
    if (!alive) return 0;

    SendCmd("ATE0", "OK", 500);                 /* 关回显 */
    SendCmd("AT+CWMODE=1", "OK", 1000);         /* Station 模式 */
    SendCmd("AT+CIPMUX=0", "OK", 500);          /* 单连接 */

    /* 连接 ESP32-S3 热点, 最长等15秒 */
    if (!SendCmd("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"", "OK", 15000)) return 0;

    SendCmd("AT+CIPCLOSE", "OK", 500);          /* 先关旧连接, 失败可忽略 */

    /* 建立 TCP 连接 */
    if (!SendCmd("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT, "OK", 5000)) return 0;

    return 1;
}

/* ---------- 发送按键信号 ---------- */
uint8_t ESP8266_SendBtn(void)
{
    Rx_Clear();
    UART1_SendString("AT+CIPSEND=4\r\n");       /* 定长发送4字节: B T N \n */
    if (!Wait_Ack(">", 2000)) return 0;         /* 等透传提示符 */

    Rx_Clear();
    UART1_SendString("BTN\n");
    if (!Wait_Ack("SEND OK", 3000)) return 0;
    return 1;
}

/* ---------- 等待LED状态回执 ---------- */
/* ESP32 收到 BTN 后回传 "LED:ON" / "LED:OFF"
 * 返回 1=亮, 0=灭, 0xFF=超时未收到 */
uint8_t ESP8266_WaitLedAck(void)
{
    uint32_t start = GetTick();
    while (GetTick() - start < 1000)
    {
        if (strstr(RxBuf, "LED:ON"))  return 1;
        if (strstr(RxBuf, "LED:OFF")) return 0;
    }
    return 0xFF;
}

/* ---------- 掉线检测 ---------- */
uint8_t ESP8266_IsClosed(void)
{
    if (strstr(RxBuf, "CLOSED"))          return 1;
    if (strstr(RxBuf, "WIFI DISCONNECT")) return 1;
    return 0;
}
