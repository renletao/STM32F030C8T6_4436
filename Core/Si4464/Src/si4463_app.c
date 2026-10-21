/**
 * @file    si4463_app.c
 * @brief   Si4463 应用层实现（基于官方 Si446x API 库 + STM32 HAL）
 */

#include <string.h>

#include "compiler_defs.h"
#include "si4463_port.h"
#include "si4463_app.h"

#include "radio_hal.h"
#include "radio_comm.h"
#include "si446x_api_lib.h"
#include "si446x_defs.h"
#include "radio_config.h"        /* WDS 生成的射频配置 */

/* WDS 生成的配置数组（严格按 si446x_configuration_init() 的格式：
   LEN | 数据…… ，最后以 0x00 结尾）。第一条就是 POWER_UP。 */
static const U8 Radio_Configuration[] = RADIO_CONFIGURATION_DATA_ARRAY;

/* 最近一次读到（并已清除）的中断挂起位 */
static U8 s_ph_pend;
static U8 s_modem_pend;
static U8 s_chip_pend;

/* ------------------------------------------------------------------ */
/* 内部：读中断并清除                                                  */
/* 注意：必须把挂起位写回 GET_INT_STATUS 的清除参数，nIRQ 才会回高。      */
/* 只读不清的话 nIRQ 一直是低，看起来像卡死。                            */
/* ------------------------------------------------------------------ */
static void si4463_irq_read_clear(void)
{
    si446x_get_int_status(0u, 0u, 0u);          /* 先读 */
    s_ph_pend    = Si446xCmd.GET_INT_STATUS.PH_PEND;
    s_modem_pend = Si446xCmd.GET_INT_STATUS.MODEM_PEND;
    s_chip_pend  = Si446xCmd.GET_INT_STATUS.CHIP_PEND;
    si446x_get_int_status(s_ph_pend, s_modem_pend, s_chip_pend);  /* 再清 */
}

/* ------------------------------------------------------------------ */
/* 内部：等 nIRQ 变低                                                  */
/* ------------------------------------------------------------------ */
static bool si4463_wait_irq(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();

    for (;;)
    {
#ifdef SI4463_NO_NIRQ_PIN
        /* 没接 nIRQ 引脚时的退路：轮询中断状态寄存器。
           注意这会持续占用 SPI，功耗和效率都不如用引脚，仅应急用。 */
        si446x_get_int_status(0u, 0u, 0u);
        if (Si446xCmd.GET_INT_STATUS.INT_PEND != 0u)
        {
            return true;
        }
#else
        if (!si4463_irq_asserted())
        {
            return true;
        }
#endif
        if ((timeout_ms != 0u) && ((HAL_GetTick() - t0) >= timeout_ms))
        {
            return false;
        }
    }
}

/* ------------------------------------------------------------------ */
/* 内部：硬件复位（SDN 拉高 -> 拉低，触发 POR）                        */
/* 官方 si446x_reset() 里的延时只有几个 NOP，在 STM32 上远不够 10us，   */
/* 所以这里自己实现，用 HAL_Delay 保证时序。                            */
/* ------------------------------------------------------------------ */
static void si4463_hw_reset(void)
{
    radio_hal_SetNsel();

    radio_hal_AssertShutdown();                 /* SDN = 1 -> 关断 */
    HAL_Delay(SI4463_SDN_HIGH_MS);              /* 手册要求 >= 10us */

    radio_hal_DeassertShutdown();               /* SDN = 0 -> 触发 POR */
    HAL_Delay(SI4463_POR_MS);                   /* 等内部校准完成 */

    radio_comm_ClearCTS();
}

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */
int8_t si4463_init(void)
{
    si4463_hw_reset();

    /* 配置数组第一条是 POWER_UP，所以这里不用再单独调 si446x_power_up() */
    if (si446x_configuration_init(Radio_Configuration) != SI446X_SUCCESS)
    {
        return SI4463_ERR_CONFIG;
    }

    /* 清一次可能残留的中断，避免上电就"看起来有中断" */
    si4463_irq_read_clear();

    return SI4463_OK;
}

int8_t si4463_reset(void)
{
    return si4463_init();
}

/* ------------------------------------------------------------------ */
/* 探活                                                               */
/* ------------------------------------------------------------------ */
int8_t si4463_part_info(uint16_t *part, uint8_t *rev)
{
    if ((part == NULL) || (rev == NULL))
    {
        return SI4463_ERR_PARAM;
    }

    si446x_part_info();

    *part = Si446xCmd.PART_INFO.PART.U16;
    *rev  = Si446xCmd.PART_INFO.CHIPREV;

    return SI4463_OK;
}

bool si4463_irq_asserted(void)
{
    return (radio_hal_NirqLevel() == (BIT)FALSE) ? true : false;
}

/* ------------------------------------------------------------------ */
/* 发送                                                              */
/* ------------------------------------------------------------------ */
int8_t si4463_tx(uint8_t channel, const uint8_t *data, uint8_t len)
{
    U8 frame[SI4463_MAX_PAYLOAD + 1u];

    if ((data == NULL) || (len == 0u) || (len > SI4463_MAX_PAYLOAD))
    {
        return SI4463_ERR_PARAM;
    }

    /* 本工程的 radio_config.h 是"变长包"配置（FIELD_1 长度 1、FIELD_2 长度 0x1FFF），
       FIFO 首字节必须是长度。若你改成定长包，请去掉这一行并直接写 payload。 */
    frame[0] = len;
    (void)memcpy(&frame[1], data, (size_t)len);

    si446x_write_tx_fifo((U8)(len + 1u), frame);
    si446x_start_tx(channel, SI4463_TX_DONE_STATE, 0u);

    if (!si4463_wait_irq(SI4463_TX_TIMEOUT_MS))
    {
        return SI4463_ERR_TIMEOUT;
    }

    si4463_irq_read_clear();

    if ((s_chip_pend & SI446X_CMD_GET_INT_STATUS_REP_FIFO_UNDERFLOW_OVERFLOW_ERROR_PEND_BIT) != 0u)
    {
        return SI4463_ERR_FIFO;
    }
    if ((s_ph_pend & SI446X_CMD_GET_INT_STATUS_REP_PACKET_SENT_PEND_BIT) == 0u)
    {
        return SI4463_ERR_TX;
    }

    return SI4463_OK;
}

/* ------------------------------------------------------------------ */
/* 接收                                                              */
/* ------------------------------------------------------------------ */
int8_t si4463_rx_start(uint8_t channel)
{
    /* RX_LEN = 0：按包处理器配置来 */
    si446x_start_rx(channel, 0u, 0u, 0u, 0u, 0u);
    return SI4463_OK;
}

int16_t si4463_rx_fetch(uint8_t *buf, uint8_t max_len, uint8_t *out_len,
                        uint32_t timeout_ms)
{
    U8 tmp[SI4463_MAX_PAYLOAD + 1u];
    U8 count;
    U8 n;

    if ((buf == NULL) || (out_len == NULL) || (max_len == 0u))
    {
        return SI4463_ERR_PARAM;
    }

    if (!si4463_wait_irq(timeout_ms))
    {
        return SI4463_ERR_TIMEOUT;
    }

    si4463_irq_read_clear();

    if ((s_ph_pend & SI446X_CMD_GET_INT_STATUS_REP_CRC_ERROR_PEND_BIT) != 0u)
    {
        return SI4463_ERR_CRC;
    }
    if ((s_ph_pend & SI446X_CMD_GET_INT_STATUS_REP_PACKET_RX_PEND_BIT) == 0u)
    {
        return SI4463_ERR_NOPKT;
    }

    si446x_fifo_info(0u);                       /* 参数 0 = 不复位 FIFO，只读计数 */
    count = Si446xCmd.FIFO_INFO.RX_FIFO_COUNT;

    if (count == 0u)
    {
        return SI4463_ERR_NOPKT;
    }
    if (count > (U8)(SI4463_MAX_PAYLOAD + 1u))
    {
        count = (U8)(SI4463_MAX_PAYLOAD + 1u);  /* 防止缓冲溢出 */
    }

    si446x_read_rx_fifo(count, tmp);            /* tmp[0] = 长度字节 */

    n = tmp[0];
    if (n > max_len)
    {
        n = max_len;
    }
    if (n > (U8)(count - 1u))
    {
        n = (U8)(count - 1u);
    }
    (void)memcpy(buf, &tmp[1], (size_t)n);
    *out_len = n;

    return (int16_t)n;
}

/* ------------------------------------------------------------------ */
/* RSSI                                                              */
/* ------------------------------------------------------------------ */
int16_t si4463_rssi_dbm(void)
{
    U8 rssi;

    si446x_get_modem_status(0u);
    rssi = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;

    /* Si446x: RSSI_dBm = RSSI/2 - 130 */
    return (int16_t)(((int16_t)rssi / 2) - 130);
}
