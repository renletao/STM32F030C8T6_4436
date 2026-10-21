/**
 * @file    radio_hal.c
 * @brief   Si446x 官方库的硬件适配层 —— STM32 HAL 实现
 *
 * 官方库（si446x_api_lib.c / radio_comm.c）通过下面这几个函数访问硬件。
 * 你只需要实现这一层，官方协议、命令、CTS 轮询逻辑全部照用。
 *
 * 说明：本文件替换掉官方自带的 radio_hal.c（那份是 STM8 的，
 *      里面有 #include <stm8s.h> 和 GPIO_WriteHigh(GPIOC,...)）。
 */

#include "compiler_defs.h"
#include "si4463_port.h"
#include "radio_hal.h"

/* ---------------------------------------------------------------- */
/* 片选（nSEL，低有效）                                              */
/* ---------------------------------------------------------------- */
void radio_hal_ClearNsel(void)
{
    HAL_GPIO_WritePin(SI4463_NSEL_PORT, SI4463_NSEL_PIN, GPIO_PIN_RESET);
}

void radio_hal_SetNsel(void)
{
    HAL_GPIO_WritePin(SI4463_NSEL_PORT, SI4463_NSEL_PIN, GPIO_PIN_SET);
}

/* ---------------------------------------------------------------- */
/* 关断脚（SDN，高 = 关断）                                          */
/* ---------------------------------------------------------------- */
void radio_hal_AssertShutdown(void)
{
    HAL_GPIO_WritePin(SI4463_SDN_PORT, SI4463_SDN_PIN, GPIO_PIN_SET);
}

void radio_hal_DeassertShutdown(void)
{
    HAL_GPIO_WritePin(SI4463_SDN_PORT, SI4463_SDN_PIN, GPIO_PIN_RESET);
}

/* ---------------------------------------------------------------- */
/* 中断脚：返回 TRUE = nIRQ 为高（无中断），FALSE = nIRQ 为低（有中断）*/
/* 官方库的用法是 `if (radio_hal_NirqLevel() == 0)`，所以这里返回引脚电平。*/
/* ---------------------------------------------------------------- */
BIT radio_hal_NirqLevel(void)
{
    return (HAL_GPIO_ReadPin(SI4463_NIRQ_PORT, SI4463_NIRQ_PIN) == GPIO_PIN_SET)
           ? (BIT)TRUE : (BIT)FALSE;
}

/* ---------------------------------------------------------------- */
/* SPI 收发                                                          */
/* 注意：CubeMX 里 SPI 必须配成                                          */
/*   Mode 0（CPOL = Low, CPHA = 1 Edge）、MSB first、8 bit、NSS = Software*/
/*   时钟 ≤ 10 MHz（Si4463 上限），建议先用 1–4 MHz 调通                */
/* ---------------------------------------------------------------- */
U8 SPI_RW(U8 byte)
{
    U8 rx = 0xFFu;

    if (HAL_SPI_TransmitReceive(SI4463_SPI, &byte, &rx, 1u,
                                SI4463_SPI_TIMEOUT_MS) != HAL_OK)
    {
        return 0xFFu;   /* 失败时返回 0xFF，调用方的 CTS 轮询会超时并报错 */
    }
    return rx;
}

void radio_hal_SpiWriteByte(U8 byteToWrite)
{
    (void)SPI_RW(byteToWrite);
}

U8 radio_hal_SpiReadByte(void)
{
    return SPI_RW(0xFFu);          /* 读的时候发 0xFF（dummy） */
}

/* 连续写：用 HAL 的块传输，比逐字节快很多（写 FIFO 时差别明显） */
void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
    if ((byteCount == 0u) || (pData == NULL))
    {
        return;
    }
    (void)HAL_SPI_Transmit(SI4463_SPI, pData, (uint16_t)byteCount,
                           SI4463_SPI_TIMEOUT_MS);
}

/* 连续读：HAL 会在发送端自动填 0xFF，正好符合 Si4463 的读时序 */
void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
    if ((byteCount == 0u) || (pData == NULL))
    {
        return;
    }
    (void)HAL_SPI_Receive(SI4463_SPI, pData, (uint16_t)byteCount,
                          SI4463_SPI_TIMEOUT_MS);
}
