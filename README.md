# STM32F030C8T6 — USART1 + GPIO 模板工程

基于 STM32CubeMX / STM32CubeIDE 生成的 **STM32F030C8T6** 裸机（HAL）工程模板。
时钟、串口、GPIO 已完成初始化，可直接在 `USER CODE` 区内编写业务代码。

## 硬件

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F030C8Tx（Cortex-M0，64 KB Flash / 8 KB RAM） |
| 封装 | LQFP48 |
| 系统时钟 | 48 MHz（HSI 8 MHz / 2 → PLL ×12） |
| 调试接口 | SWD |
| 串口 | USART1 异步，115200-8-N-1，**PA9 (TX) / PA10 (RX)** |

## 已初始化的外设

- **RCC** — HSI + PLL，SYSCLK = HCLK = PCLK1 = 48 MHz，USART1 时钟源 `PCLK1`
- **GPIO** — `MX_GPIO_Init()`
- **USART1** — `MX_USART1_UART_Init()`，115200 bps
- **SysTick** — 1 ms 时基（`HAL_Init()` 默认）

## 目录结构

```
├── Core/
│   ├── Inc/          # 头文件（main.h、外设初始化声明、HAL 配置）
│   ├── Src/          # 源文件（main.c、外设初始化、中断、syscalls）
│   └── Startup/      # 启动汇编 startup_stm32f030c8tx.s
├── Drivers/
│   ├── CMSIS/        # CMSIS Core + STM32F0xx 器件头文件
│   └── STM32F0xx_HAL_Driver/
├── STM32F030C8T6_4436.ioc      # CubeMX 工程文件
├── STM32F030C8TX_FLASH.ld      # GCC 链接脚本
├── .cproject / .project        # STM32CubeIDE 工程配置
└── .gitignore                  # 忽略 build/ 等编译产物
```

## 编译与烧录

1. 用 **STM32CubeIDE** 打开本目录：`File → Open Projects from File System…`
2. `Project → Build Project`（默认生成到 `build/`，该目录已被 git 忽略）
3. `Run → Debug`，通过 ST-Link / DAP-Link 以 SWD 方式下载

命令行（需 `arm-none-eabi-gcc`，或直接使用 CubeIDE 的构建目录）：

```bash
make -C Debug all      # 若使用 CubeIDE 生成的 Makefile 工程
```

## 使用说明

`main.c` 中的 `/* USER CODE BEGIN ... */` 与 `/* USER CODE END ... */` 之间的内容
在 CubeMX 重新生成代码时会保留，业务逻辑请写在这些区域内。

## 许可证

- `Drivers/CMSIS/` 与 `Drivers/STM32F0xx_HAL_Driver/` 为 STMicroelectronics 版权，
  遵循各自目录下的 `LICENSE.txt`（BSD-3-Clause / Apache-2.0）。
- 本工程其余部分可自由使用。
