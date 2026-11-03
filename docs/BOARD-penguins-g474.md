# Penguins-G474 板卡引脚与时钟配方

本模板默认面向 **Penguins-G474** 开发板。板卡资料与配套例程在
`D:\Projects\GitHub\NUEDC_2027`（`X Pulse开发板参考资料/` 与 `G474/G474_*` 系列教程工程），
那些工程用 **Keil MDK + CubeMX**；本模板把它们的外设事实提炼成 PlatformIO 工程。

## 1. 主控与存储

| 项 | 值 |
| --- | --- |
| 型号 | STM32G474VET6 |
| 封装 | LQFP100 |
| 内核 / 主频 | Cortex-M4F，最高 170 MHz |
| Flash / RAM | 512 KB / 128 KB（含 32 KB CCM SRAM） |
| 模板板级定义 | `boards/genericSTM32G474VE.json`（`mcu = stm32g474vet6`） |

## 2. 板上外设引脚

引脚定义来自参考工程 `G474/G474_01_gpio/Project.ioc` 与 `Core/Inc/main.h`：

| 功能 | 引脚 | 电气 | 说明 |
| --- | --- | --- | --- |
| LED_R | PC13 | 推挽输出，**低电平点亮** | 本模板默认闪这一颗 |
| LED_G | PC14 | 推挽输出，低电平点亮 | 与 LSE 的 OSC32_IN 复用 |
| LED_B | PC15 | 推挽输出，低电平点亮 | 与 LSE 的 OSC32_OUT 复用 |
| KEY_S1…S4 | PC0…PC3 | 输入，无内部上下拉 | 参考工程按**按下为低**使用，板上应有外部上拉 |
| HSE 晶振 | PF0 / PF1 | 外部晶振输入/输出 | 24 MHz，`RCC_OSC_IN` / `RCC_OSC_OUT` |
| SWD | PA13 / PA14 | `SYS_JTMS-SWDIO` / `SYS_JTCK-SWCLK` | 模板在 `.ioc` 里显式声明，避免被重生成时释放 |
| USB 转串口 | — | 板载 CH340 类芯片 | 训练仓库提供 CH341SER 驱动；与本模板固件无关 |

注意：

- **PC13/PC14/PC15 属备份域引脚，驱动能力弱**，只适合指示灯这类低速用途，不要在这里输出高速信号。
- 若要使用 LSE（32.768 kHz），必须放弃 LED_G / LED_B，二者共用 OSC32 引脚。
- 模板只启用 PC13；LED_G/B 与按键需要在 CubeMX 里自行添加。

## 3. 时钟配方

### 3.1 模板默认：HSI 16 MHz（当前 `.ioc`）

- `RCC_OSCILLATORTYPE_HSI`，`PLLState = RCC_PLL_NONE`
- `SYSCLKSource = RCC_SYSCLKSOURCE_HSI`，AHB/APB1/APB2 均为 DIV1
- `FLASH_LATENCY_0`，`HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)`

选它的理由：上电即可跑、不依赖外部晶振是否起振，第一次烧录最容易定位问题。
代价是没有 FPU 高频余量、外设时钟都只有 16 MHz。

### 3.2 升级到 170 MHz（参考工程在用的配置）

参考工程 `G474_01_gpio/Project.ioc` 的实际取值：

```ini
RCC.HSE_VALUE=24000000
RCC.PLLSourceVirtual=RCC_PLLSOURCE_HSE
RCC.PLLM=RCC_PLLM_DIV6      # 24 MHz / 6 = 4 MHz（VCO 输入）
RCC.PLLN=85                 # 4 MHz × 85 = 340 MHz（VCO）
# PLLR = /2 → 170 MHz（CubeMX 默认 R=2，未单列在 IPParameters 里）
RCC.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK
```

**推荐做法**：在 CubeMX GUI 里 *RCC → HSE = Crystal Resonator*，*Clock Configuration*
把 PLL Source 选 HSE、M=6、N=85、R=2，让工具自己算 `FLASH_LATENCY` 与电压档，
然后 GENERATE CODE。手改 `.ioc` 时别忘了 `RCC.IPParameters` 白名单
（见 `TEMPLATE-PLACEHOLDERS.md` D3）。

170 MHz 下必须同时满足：

- 电压档 `PWR_REGULATOR_VOLTAGE_SCALE1`（Boost 模式）；
- Flash 等待周期由 CubeMX 自动填成 `FLASH_LATENCY_4`，**不要手写更小的值**；
- APB1 最高 170 MHz、APB2 最高 170 MHz，但定时器时钟会按分频倍频，配外设时按 `RCC.APB1TimFreq_Value` 算。

### 3.3 200 MHz 超频（不属于模板承诺）

训练仓库用 `mclk_modifier.bat` 在 Keil 工程里把主频拉到 200 MHz。这超出 ST 数据手册的
170 MHz 上限，属于板级实验：

- 模板**不提供**该配置，也不保证 PlatformIO 构建出的固件在 200 MHz 下稳定；
- 如果确实要做，先把 170 MHz 配方跑稳（L5 实板验证），再单独评估供电、Flash 等待周期与温度；
- 不要把它写进模板默认值，否则复制模板的人会在不知情的情况下超频。

## 4. 与参考工程的结构差异

| 项 | 训练仓库（`NUEDC_2027/G474`） | 本模板 |
| --- | --- | --- |
| 工具链 | Keil MDK（AC6） | PlatformIO + GCC ARM |
| CubeMX 工具链选项 | `MDK-ARM V5.32` | `Makefile`（只用来产出 Core/） |
| `CoupleFile` | `true`（生成 `gpio.c` / `tim.c`） | `false`（全部 `MX_xxx_Init()` 在 `main.c`） |
| 时钟 | HSE 24 MHz → 170 MHz（部分 200 MHz） | HSI 16 MHz |
| 外设 | 按章节递增（GPIO/LCD/UART/ADC/DAC…） | 只有 PC13 + SWD |
| 库来源 | 仓库内 `Drivers/` 全量提交 | PlatformIO `framework-stm32cubeg4@1.6.3` |

移植参考工程的外设时，把 CubeMX 里的配置照抄到本模板的 `.ioc`，重新生成即可；
业务代码放进 `USER CODE` 块或 `Core/Src` 下新增的独立文件（会被 `src_dir` 自动编译）。
