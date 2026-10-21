/**
 * @file    si4463_port.h
 * @brief   Si4463 移植配置：SPI 实例、引脚、超时
 *
 * 目标板：STM32F030C8T6_4436
 *   SPI2  : PB13 SCK / PB14 MISO / PB15 MOSI（AF0，Mode 0，3 MHz）
 *   nSEL  : PB12（输出，初值高）
 *   SDN   : PB1 （输出，初值低，低=工作）
 *   nIRQ  : PB10（上拉输入 / EXTI 下降沿）
 *
 * 换板子/换 SPI：只改这个文件。
 */

#ifndef SI4463_PORT_H
#define SI4463_PORT_H

/* ===== 1. MCU 系列头文件 ===== */
#include "stm32f0xx_hal.h"

/* ===== 2. SPI 句柄 ===== */
extern SPI_HandleTypeDef hspi2;
#define SI4463_SPI                (&hspi2)

/* ===== 3. 引脚 ===== */
#define SI4463_NSEL_PORT          GPIOB
#define SI4463_NSEL_PIN           GPIO_PIN_12

#define SI4463_SDN_PORT           GPIOB
#define SI4463_SDN_PIN            GPIO_PIN_1

#define SI4463_NIRQ_PORT          GPIOB
#define SI4463_NIRQ_PIN           GPIO_PIN_10

/* ===== 4. 时序参数（物理时间，和主频无关） ===== */
#define SI4463_SPI_TIMEOUT_MS     10u   /* 单次 SPI 传输超时 */
#define SI4463_SDN_HIGH_MS        20u   /* 手册要求 >=10us，这里给足 */
#define SI4463_POR_MS             20u   /* SDN 释放后等 POR + 内部校准 */

/* ===== 5. 应用层参数 ===== */
#define SI4463_MAX_PAYLOAD        63u   /* FIFO 64 字节，变长包首字节占 1 */
#define SI4463_TX_TIMEOUT_MS      1000u /* 等 TX 完成中断 */
#define SI4463_RX_TIMEOUT_MS      0u    /* 0 = 一直等 */
#define SI4463_TX_DONE_STATE      0x30u /* 发完回 READY；0x00 = 保持 TX */

/* 没接 nIRQ 时打开这个宏：驱动改为轮询中断状态寄存器 */
/* #define SI4463_NO_NIRQ_PIN */

#endif /* SI4463_PORT_H */
