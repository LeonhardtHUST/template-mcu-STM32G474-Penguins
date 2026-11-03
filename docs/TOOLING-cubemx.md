# 用 AI/CLI 修改 STM32CubeMX 配置：本机工具与 G4 实测

本文记录**本机（Leon 的 Windows 开发机）**已有的 CubeMX 自动化能力，以及把本模板
从零生成出来时**实际观察到的行为**。目的是让任何 agent（不限客户端）复用同一套结论，
不必重新试错。未验证的推测会明确标注。

---

## 1. 本机工具现状

| 项 | 值 |
| -- | -- |
| CubeMX 可执行文件 | `C:\ST\STM32CubeMX\STM32CubeMX.exe`（**不在** `C:\Program Files\STMicroelectronics\...`） |
| 实际版本 | exe 自报 **6.18.1**；`.ioc` 写回 `MxCube.Version=6.18.1`、`MxDb.Version=DB.6.0.181` |
| 自带 JRE | `C:\ST\STM32CubeMX\jre\bin\java.exe` |
| G4 固件包 | `C:\Users\Leon\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.3` |
| 器件数据库 | `C:\ST\STM32CubeMX\db\mcu\STM32G474V(B-C-E)Tx.xml` 等，**信号名以此为准** |
| PlatformIO | Core 6.2.0；`ststm32@20.0.0`；`framework-stm32cubeg4@1.6.3`（与 FW_G4 同版本） |
| 主机 GCC | MinGW gcc 15.2.0（仅用于 `test/` 回归） |

> 判断 CubeMX 版本**看 exe 属性或运行日志**，不要相信 `.ioc` 里的版本戳：
> 旧模板会把自己的历史戳原样继承下来。本模板的 6.18.1 是本次生成写回的真实值。

## 2. 直接调 CLI

```powershell
& "C:\ST\STM32CubeMX\jre\bin\java.exe" -jar "C:\ST\STM32CubeMX\STM32CubeMX.exe" -q "D:\path\to\script.mx"
```

脚本每行一条命令，本模板用到的：

```text
load STM32G474VETx                 # 从 MCU 裸建（不带 .ioc）
config load <相对或绝对 .ioc 路径>  # 加载已有配置
project name stm32-g474-template
project toolchain Makefile
project path <工程根目录>
project generate
exit
```

单次生成的耗时主要花在 JVM 启动与 CMSIS 拷贝上，本机约 **1 分钟量级**；超时阈值不要设太小。

## 3. 本次生成模板时实测到的行为

以下每条都在 2026-11-03 的生成日志或生成物里核对过。

### 3.1 `project path` 会再追加一层工程名

`project path D:\scratch\gen` + `project name foo` → 产物落在 `D:\scratch\gen\foo\`。
想让文件落在仓库根目录，要么生成后上移，要么在 `.ioc` 已存在时用 `config load` + `project generate`
（后者按 `.ioc` 所在目录生成）。

### 3.2 原地生成让 `.mxproject` 变成相对路径

先 `project path` 指到别处生成，`.mxproject` 的 `[PreviousUsedMakefileFiles]` 会写入**绝对路径**；
把 `.ioc` 放进仓库根目录后执行 `config load` + `project generate`，同文件变成
`Core\Src\main.c` 这类相对路径。**提交前确认 `.mxproject` 里没有盘符路径**（`grep "D:"` 应为 0）。

### 3.3 `AskForMigrate` 会被写回 `true`

模板里提交的是 `ProjectManager.AskForMigrate=false`，但 CubeMX 每次生成/保存都会把它改回
`true`。无头模式如果卡在 `Begin LoadConfig()` 后不再输出，先把它改成 `false` 再跑
（这是模态迁移框在等人点）。

### 3.4 `syscalls.c` / `sysmem.c` 会被写成叠加路径

生成日志出现：

```text
FileStreamsReadnWrite: java.io.FileNotFoundException:
  <工程目录>\D:\Projects\...\Core\Src\syscalls.c (拒绝访问。)
```

这两个文件因此**不会更新**（其余文件正常）。它们是标准的 newlib 桩，本模板从 F103 模板
复制了等价文件；要更新就从 `STM32Cube_FW_G4_V1.6.3` 的模板工程里取。

### 3.5 GPIO 参数要写进 `GPIOParameters` 白名单

本次把 PC13 设成"输出 + 初始高电平"，生效的写法是**两行一起**：

```ini
PC13.GPIOParameters=PinState
PC13.PinState=GPIO_PIN_SET
PC13.Signal=GPIO_Output
```

重新生成后 `main.c` 里确实出现 `HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)`。
同理，外设参数要进 `<IP>.IPParameters`、引脚清单要同步 `Mcu.Pin*` 与 `Mcu.PinsNb`。

### 3.6 G474 最小工程也会有一条 `VP_RIF_VS_RIF1` 诊断

即使 `.ioc` 里只有 NVIC/RCC/SYS，生成日志仍有：

```text
[INFO] ImportTextPane:234 - (OptionalMessage_ERROR) Pin5 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU
```

- 它**不是** `Cannot map signal`，与本工程的 PC13 / SWD 配置无关；
- 加/减普通 GPIO 不会让它消失（本次两次生成都有）；
- 判定"配置是否生效"仍以**生成物**为准：日志无 `Cannot map` + `main.c` 里有对应
  `MX_xxx_Init()`。来源未定位，不要为它删外设。

### 3.7 其它已知噪音

- `Updater plugin not ready yet` / `Updater is busy`：更新器启动噪音，不阻塞生成；
- 首次在新版本上运行可能弹一次迁移/提示框；
- `config saveas` 会同时改 `ProjectName` / `ProjectFileName`，想原地规范化用 `config save`。

## 4. 可选：`vscode-cube-mcp`（本机已全局安装）

| 项 | 值 |
| -- | -- |
| 包 | `vscode-cube-mcp` v0.4.2（MIT，PyPI） |
| 可执行文件 | `C:\Users\Leon\.local\bin\vscode-cube-mcp.exe` |
| 环境变量（用户级） | `ST_CUBEMX_EXE=C:\ST\STM32CubeMX\STM32CubeMX.exe`、`ST_CUBEMX_ALLOWED_ROOTS=D:\Projects;C:\Users\Leon\WorkBuddy`、`ST_CUBEMX_TIMEOUT=300` |

它提供 `cubemx_load`（只读回读）、`cubemx_configure`（带备份写回）、`cubemx_generate`、
`cubemx_new_project`、`cubemx_remove_peripheral` 等工具，并强制 `.ioc` 路径白名单。
`cubemx_configure` 是**纯脚本透传**，不做文本注入；真正能配外设的是 `cubemx_new_project`
的 `commands` 参数。

`ALLOWED_ROOTS` 是安全白名单：换工作目录时要一起改，否则会报"路径不在允许范围内"。

## 5. 不推荐再装的方案

| 方案 | 结论 | 理由 |
| --- | --- | --- |
| `stm32cubemx-mcp`（Wafleem） | 不装 | 代码生成走 STM32CubeIDE，与 CubeMX CLI 不是一回事；能力已被 `vscode-cube-mcp` 覆盖 |
| `stm32pio`（PyPI） | 不装 | 2021 年后未维护，且面向"用 CubeMX 重建 PIO 工程"，会与本模板既有的 `platformio.ini` 冲突 |
| `STM32Cube-MCP`（brovk2008） | 不装 | 大部分工具是烧录/GDB/串口 GUI 自动化，远超"改配置"需求 |
