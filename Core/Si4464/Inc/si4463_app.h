/**
 * @file    si4463_app.h
 * @brief   Si4463 应用层接口（基于官方 Si446x API 库）
 *
 * 依赖：
 *   - drivers/official-si446x-api-lib/ 里的 si446x_api_lib.c / radio_comm.c
 *   - 本目录 Src/radio_hal.c（STM32 HAL 适配层）
 *   - 编译时定义：RADIO_DRIVER_EXTENDED_SUPPORT（必需，FIFO 读写函数）
 *                 RADIO_DRIVER_FULL_SUPPORT    （可选，RSSI/状态查询）
 */

#ifndef SI4463_APP_H
#define SI4463_APP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 返回值：0 = 成功，负数 = 错误 */
#define SI4463_OK             0
#define SI4463_ERR_PARAM    (-1)   /* 参数错误 */
#define SI4463_ERR_CONFIG   (-2)   /* 写配置失败（CTS 超时/命令错误） */
#define SI4463_ERR_TIMEOUT  (-3)   /* 等 nIRQ 超时 */
#define SI4463_ERR_TX       (-4)   /* 没有收到 PACKET_SENT */
#define SI4463_ERR_CRC      (-5)   /* 收到包但 CRC 错 */
#define SI4463_ERR_NOPKT    (-6)   /* 没有收到包（可能是无效同步字/前导码） */
#define SI4463_ERR_FIFO     (-7)   /* FIFO 上溢/下溢 */

/**
 * @brief  初始化：硬件复位 + 上电 + 写 WDS 配置
 * @retval SI4463_OK 或错误码
 * @note   调用前请确保：SPI 已初始化、nSEL/SDN 引脚已配成推挽输出、
 *         nIRQ 已配成输入（上拉）。
 */
int8_t si4463_init(void);

/**
 * @brief  读芯片型号与版本（探活）
 * @param  part 输出：0x4463 表示 Si4463
 * @param  rev  输出：芯片版本（如 0x03 = B1/C）
 */
int8_t si4463_part_info(uint16_t *part, uint8_t *rev);

/**
 * @brief  nIRQ 是否为低（有中断未取）
 */
bool si4463_irq_asserted(void);

/**
 * @brief  发送一包（阻塞，直到发完或超时）
 * @param  channel 信道号（WDS 里配的频率表序号，单频应用填 0）
 * @param  data    载荷
 * @param  len     载荷长度（1 ~ SI4463_MAX_PAYLOAD）
 * @note   FIFO 会自动加上"长度字节"（本工程配置为变长包）。
 */
int8_t si4463_tx(uint8_t channel, const uint8_t *data, uint8_t len);

/**
 * @brief  启动接收
 */
int8_t si4463_rx_start(uint8_t channel);

/**
 * @brief  等待并取出一包（阻塞）
 * @param  buf        输出缓冲区
 * @param  max_len    buf 容量
 * @param  out_len    输出：实际载荷长度
 * @param  timeout_ms 0 = 永久等待
 * @retval >0 表示收到的字节数，<0 为错误码
 */
int16_t si4463_rx_fetch(uint8_t *buf, uint8_t max_len, uint8_t *out_len,
                        uint32_t timeout_ms);

/**
 * @brief  读当前 RSSI（dBm），需要 RADIO_DRIVER_FULL_SUPPORT
 */
int16_t si4463_rssi_dbm(void);

/**
 * @brief  软件复位（重新走一遍 SDN -> POR -> 写配置）
 */
int8_t si4463_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* SI4463_APP_H */
