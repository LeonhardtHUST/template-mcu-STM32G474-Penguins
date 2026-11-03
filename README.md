# template-mcu-STM32G474-series

STM32G474 **最小工程模板**，默认 HSI 16 MHz、PC13 闪灯和 ST-Link/SWD。
默认面向 **Penguins-G474** 开发板（STM32G474VET6，LQFP100，512K Flash / 128K RAM）；
换芯片/换板只需改三处，见 [`TEMPLATE-PLACEHOLDERS.md`](TEMPLATE-PLACEHOLDERS.md)。
沿用工程文件名 `stm32-g474-template.ioc`，复制模板后可继续直接使用。

本项目采用 **STM32CubeMX + PlatformIO + VS Code** 混合工作流：

- **STM32CubeMX**：管脚/时钟/外设配置，生成 `Core/Inc`、`Core/Src`
- **PlatformIO**：编译、烧录、调试（HAL / CMSIS / 启动文件 / 链接脚本由包提供）
- **VS Code**：编辑器与调试前端

## 快速开始

```bash
# 1. 安装 VS Code 推荐扩展（打开工程时会提示安装）
#    PlatformIO IDE / STM32 VS Code Extension / Cortex-Debug / C/C++

# 2. 编译（首次会自动下载 ststm32 平台、CubeG4 固件包与工具链）
pio run

# 3. 烧录（ST-Link 接 SWD：SWDIO-PA13、SWCLK-PA14、GND 共地，目标板供电）
pio run -t upload

# 4. 调试：VS Code 运行面板选择 "Debug (OpenOCD / ST-Link)"
```

常用命令：

| 命令 | 说明 |
| --- | --- |
| `pio run -e genericSTM32G474VE` | 构建默认环境 |
| `pio run -t upload` | 烧录 |
| `pio run -t clean` | 清理构建产物 |
| `pio device monitor` | 串口监视（默认 115200，需自行启用 USART） |

## 目录结构

```text
Core/Inc                 # CubeMX 生成的用户头文件（含 stm32g4xx_hal_conf.h）
Core/Src                 # CubeMX 生成的用户源码（PlatformIO 的 src_dir）
Drivers/                 # CubeMX 生成的 HAL/CMSIS 库（不参与 PIO 编译，已 git 忽略）
boards/                  # 自定义板级定义（PlatformIO 官方没有 G474VET6 的 board）
scripts/                 # CubeMX 命令行脚本 + OpenOCD 探针配置
test/                    # 系统 GCC 主机回归（不使用 pio test / Unity）
.vscode/                 # 任务、调试、推荐扩展配置
platformio.ini           # PlatformIO 工程配置
stm32-g474-template.ioc  # CubeMX 配置（唯一事实来源）
AGENTS.md                # agent 工作约束（提交规范 / 硬约束 / 验证关卡）
TEMPLATE-PLACEHOLDERS.md # 复制模板后需要人工修改的占位符清单
docs/                    # 板卡引脚、时钟配方与 CubeMX 工具说明
```

## 修改外设配置的流程

1. VS Code 任务面板运行 **STM32CubeMX: Open .ioc**（或直接用 CubeMX 打开 `.ioc`）
2. 配置完成后点 **GENERATE CODE**（工具链保持 `Makefile`，不要改目录结构）
3. 回到 VS Code，用户代码写在 `main.c` 的 `/* USER CODE BEGIN */ ... /* USER CODE END */` 之间
4. `pio run` 重新编译

无头生成（CI/脚本）可用任务 **STM32CubeMX: Generate Code (CLI)**，脚本见
`scripts/cubemx-generate.mx`。该脚本用**相对路径**加载 `.ioc`，因此工作目录必须是项目根目录
（`tasks.json` 已设 `cwd`）。本机 CubeMX 装在 `C:\ST\STM32CubeMX`，不在默认安装目录，
换机器时要同步改 `tasks.json`。

## 工作流约定（重要）

- **`.ioc` 是唯一事实来源；改它的首选方式是 CubeMX GUI。**
  手改 `.ioc` 文本可行，但 **CubeMX 对 `.ioc` 的错误是非致命的**：信号名这类枚举写错时会
  **静默丢弃**该外设（日志里只有一行 `Cannot map signal ...`），`project generate` 仍返回成功。
  所以任何 `.ioc` 改动后**必须**核验：
  ① 生成日志里没有 `Cannot map`；② 生成的 `main.c` 里有对应的 `MX_xxx_Init()`。
  每个外设/引脚还有 `IPParameters` / `GPIOParameters` 白名单，**手加的参数必须同步补进那一行**，
  否则会被静默忽略。细节见 [`TEMPLATE-PLACEHOLDERS.md`](TEMPLATE-PLACEHOLDERS.md) 的 D3–D5。
- HAL / CMSIS / 启动文件 / 链接脚本 **由 PlatformIO 包提供**，因此：
  - `Core/Src/system_stm32g4xx.c` 已在 `platformio.ini` 中排除，避免与 framework 的 CMSIS 重复定义符号；
  - `Drivers/`、`Makefile`、`startup_stm32g474xx.s`、`STM32G474xx_FLASH.ld` 不参与 PlatformIO 编译，已加入 `.gitignore`；
  - 链接脚本由 `boards/genericSTM32G474VE.json` 的 `mcu = stm32g474vet6` 自动选中
    `tool-ldscripts-ststm32/stm32g4/STM32G474VETX_FLASH.ld`，不再依赖 CubeMX 生成的链接脚本。
    改芯片型号时只改板级 `mcu`，链接脚本会跟着变。
- 新增外设后，若 `Core/Inc/stm32g4xx_hal_conf.h` 缺少对应 `HAL_XXX_MODULE_ENABLED`，
  在 CubeMX 中重新生成即可（CubeMX 会覆盖该文件，用户代码不要写在该文件里）。
- 当前时钟为 HSI 16 MHz、FLASH_LATENCY_0、电压档 SCALE1。要用 HSE 24 MHz + PLL 跑 170 MHz，
  配方与注意事项见 [`docs/BOARD-penguins-g474.md`](docs/BOARD-penguins-g474.md)。

## 模板占位符与初始条件

复制本模板后**必须人工修改**的内容（绝对路径、工程名、工具链前提、已知陷阱）统一列在
**[`TEMPLATE-PLACEHOLDERS.md`](TEMPLATE-PLACEHOLDERS.md)**。要点：

| 类别 | 例子 |
| --- | --- |
| 必须替换 | `scripts/cubemx-init.mx` 的 `__PROJECT_DIR__`；工程名 `stm32-g474-template`（六处要一起改） |
| 视硬件而定 | 探针（默认 `stlink`，可切换 `cmsis-dap`）、`boards/*.json` 的 `mcu` 与容量、CubeMX 安装路径 |
| 环境前提 | 已装 CubeMX + FW_G4 包、PlatformIO Core、主机 GCC（仅回归测试用） |
| 已知陷阱 | `formatOnSave` 会重排 `main.c`；`.ioc` 的 `*IPParameters` 白名单；`AskForMigrate` 会被写回 `true` |

agent 的工作约束（提交规范、硬约束、验证关卡）见 **[`AGENTS.md`](AGENTS.md)**。

## 故障排查

- **`pio` 命令不存在**：安装 PlatformIO IDE 扩展后重启 VS Code，或 `pip install platformio`。
  Windows 集成终端通过 [settings.json](.vscode/settings.json) 的
  `terminal.integrated.env.windows` 将默认 `penv\Scripts` 放在 PATH 开头，
  避免旧的 pio shim 抢先匹配；修改设置后新建终端，再用 `pio --version` 核对。
- **找不到下载器**：默认走 ST-Link（`upload_protocol = stlink`、`debug_tool = stlink`），
  底层调用 PlatformIO 自带的 OpenOCD + `interface/stlink.cfg`。
  Cortex-Debug 的 Debug/Attach 使用 `scripts/openocd-stlink.cfg`。
  改用 CMSIS-DAP 时，将上述两项改为 `cmsis-dap`，并把 `launch.json` 中两个
  `configFiles` 切到 `scripts/openocd-cmsis-dap.cfg`。
- **SWD 连接失败（`cannot read IDR` / `No ACK`）**：优先检查 SWDIO/SWCLK 是否接反、GND 是否共地、
  目标板是否独立供电；必要时降低 `debug_speed` 或改用「连接时复位」（`connect_assert_srst`）。
- **重复定义符号**：检查是否把 `Drivers/` 加入编译，或把 `system_stm32g4xx.c` 的排除规则删除了。
- **链接脚本不对/Flash 溢出**：确认 `boards/*.json` 的 `mcu` 与实际芯片一致（VET6 为
  `stm32g474vet6`），`maximum_size` / `maximum_ram_size` 与实际容量一致。
- **CubeMX 无头生成卡住**：多半是 `.ioc` 的 `ProjectManager.AskForMigrate=true` 弹了模态迁移框。
  改成 `false` 再跑（CubeMX 生成后会把它写回 `true`，属正常）。见
  [`docs/TOOLING-cubemx.md`](docs/TOOLING-cubemx.md)。

## 模板维护与验证

- 固定 ststm32 20.0.0、CubeG4 1.6.3、GCC ARM 7.2.1、链接脚本包 0.3.0、OpenOCD 3.1200.7。
  CubeG4 1.6.3 与本机 CubeMX 使用的 FW_G4 V1.6.3 同版本；升级时需同步验证生成代码与构建，
  避免单独升级 HAL。本机工具记录见 [CubeMX 工具说明](docs/TOOLING-cubemx.md) §1。
- 工程源码使用 C11、`-Wall -Wextra -Werror`；严格选项不施加于 HAL/CMSIS。
  每次链接输出 `.pio/build/genericSTM32G474VE/firmware.map`，便于检查段和符号布局。
  newlib-nano 默认不含浮点 printf；需要 `%f` 时，在 `platformio.ini` 中取消
  `-Wl,-u,_printf_float` 的注释并重新构建。
- `board_build.stm32cube.custom_config_header = yes` 保证使用 `Core/Inc/stm32g4xx_hal_conf.h`。
- `Core/Src/stdio_port.c` 覆盖 CubeMX 的弱 `_read` / `_write`，适用于当前 newlib 工具链。
  未提供后端时返回 `-1` 并设置 `errno = ENOSYS`。需要串口时，在独立用户源码中实现
  `int __io_putchar(int ch)` 和 `int __io_getchar(void)`：成功返回字符（0..255），失败返回负数。

主机回归测试（需要 GCC，不需要开发板）：

```bash
gcc -std=c11 -Wall -Wextra -Werror Core/Src/stdio_port.c test/stdio_port_test.c -o .pio/host-test/stdio-test && ./.pio/host-test/stdio-test
gcc -std=c11 -Wall -Wextra -Werror -DTEST_BACKEND Core/Src/stdio_port.c test/stdio_port_test.c -o .pio/host-test/stdio-backend-test && ./.pio/host-test/stdio-backend-test
pio run
```

Windows 下给输出文件加 `.exe` 后缀。

**编译通过不代替实板验证**：首次使用需检查烧录、PC13 闪灯、断点与 Attach。
本模板目前只做到 L4（生成核验）与 L3（构建），**尚未做 L5（实板）验证**；
首次上板请按上面的顺序逐项确认。
