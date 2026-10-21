/**
 * @file    compiler_defs.h
 * @brief   Si446x 官方库所需的 8051/STM8 风格类型定义 —— STM32 (ARM GCC) 版本
 *
 * 官方库原本自带一份 compiler_defs.h，里面 #include <stm8s.h>、把 U16 定义成
 * `unsigned int`。在 ARM 上 `unsigned int` 是 32 位，直接照用会让所有 16 位
 * 变量变宽（尤其 start_tx 的 TX_LEN、各种长度计算），必须换成这份。
 *
 * 用法：把本目录放在 include path 里，且**排在** drivers/official-si446x-api-lib/ 之前，
 *      或者干脆删掉官方目录里的 compiler_defs.h。
 *
 * 建议编译选项：-std=c99   （本文件用 <stdint.h>，不能用 C90）
 */

#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 官方库使用的基本类型 ---------- */
typedef _Bool    BIT;      /* 官方库把它当 0/1 使用 */
typedef uint8_t  U8;
typedef uint16_t U16;      /* ★ 关键：必须是 16 位，不能是 unsigned int */
typedef uint32_t U32;

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;

/* ---------- 官方库使用的联合体（用于 16/32 位拆分） ---------- */
typedef union UU16
{
    U16 U16;
    S16 S16;
    U8  U8[2];
    S8  S8[2];
} UU16;

typedef union UU32
{
    U32  U32;
    S32  S32;
    UU16 UU16[2];
    U16  U16[2];
    S16  S16[2];
    U8   U8[4];
    S8   S8[4];
} UU32;

/* 官方库用这两个下标访问 UU32 的字节 */
#ifndef LSB
#define LSB 0
#endif
#ifndef MSB
#define MSB 1
#endif
#ifndef b0
#define b0 0
#define b1 1
#define b2 2
#define b3 3
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ---------- NOP ---------- */
#if defined(__GNUC__)
  #define NOP() __asm volatile ("nop")
#else
  #define NOP() do { } while (0)
#endif

/* ---------- 官方库的功能开关 ----------
 * 这两个宏正常应加在工程编译选项里（-D）。
 * 为了让本工程"拷进来就能编"，这里用 #ifndef 兜底：
 * 以后你在 CubeIDE 的 Defined symbols 里也加了同样的宏，不会冲突。
 *
 *   RADIO_DRIVER_EXTENDED_SUPPORT : si446x_write_tx_fifo / read_rx_fifo / fifo_info
 *   RADIO_DRIVER_FULL_SUPPORT     : si446x_get_modem_status（RSSI）等
 */
#ifndef RADIO_DRIVER_EXTENDED_SUPPORT
  #define RADIO_DRIVER_EXTENDED_SUPPORT
#endif
#ifndef RADIO_DRIVER_FULL_SUPPORT
  #define RADIO_DRIVER_FULL_SUPPORT
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMPILER_DEFS_H */
