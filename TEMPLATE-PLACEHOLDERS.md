# 模板占位符与初始条件（PLACEHOLDERS）

本文件列出**复制本模板后必须人工修改的占位符**，以及无法靠代码消除的"初始条件"。
新建工程时请逐条过一遍；agent 拿到本模板后也应以本文件为清单先做核对。

> 约定：本模板中**没有任何**指向原作者机器的绝对路径是"可用的"——凡是绝对路径都属于占位符。

---

## A. 必须替换（阻塞项）

| # | 位置 | 当前值 / 占位符 | 应改成 | 说明 |
| - | ---- | -------------- | ------ | ---- |
| A1 | `scripts/cubemx-init.mx` 第 5 行 | `project path __PROJECT_DIR__` | 本副本的**绝对路径**，如 `D:\Projects\foo\my-board` | 该脚本用于"从 MCU 裸建工程"，CubeMX 脚本**不支持变量**，必须硬写绝对路径。仅用 `scripts/cubemx-generate.mx` 的话可以不碰它。 |
| A2 | `stm32-g474-template.ioc` | 工程名 `stm32-g474-template` | 你的工程名 | 工程名出现在**六处**：① `.ioc` 文件名 ② `ProjectManager.ProjectName` ③ `ProjectManager.ProjectFileName` ④ `.vscode/tasks.json` 的两个 `.ioc` 路径 ⑤ `scripts/cubemx-generate.mx` 的 `config load` ⑥ `README.md`。**要一起改**，否则 CubeMX 会按新名字再生成一套目录，或任务指向不存在的文件。 |
| A3 | 仓库自身 | 仓库名 `template-mcu-STM32G474-series` | 你的仓库名 | `git remote -v` 指向模板仓库时，提交会推到模板去。新建仓库后确认 remote。 |
| A4 | `README.md` 第 1 行 | `# template-mcu-STM32G474-series` | 你的项目名 | 纯展示，但会误导协作者。 |

## B. 视硬件/工具链而定（可能不用改）

| # | 位置 | 当前值 | 何时需要改 |
| - | ---- | ------ | ---------- |
| B1 | `platformio.ini` `upload_protocol` / `debug_tool`；`.vscode/launch.json` 的 `configFiles` | `stlink`；`scripts/openocd-stlink.cfg`（原生 ST-Link 驱动，SWD） | 换 CMSIS-DAP 时将两项改为 `cmsis-dap`，并把 Debug/Attach 的 `configFiles` 切到已提供的 `scripts/openocd-cmsis-dap.cfg`。 |
| B2 | `boards/genericSTM32G474VE.json` 的 `mcu` / `maximum_size` / `maximum_ram_size` / `name` | `stm32g474vet6`、524288、131072、512K/128K | 换芯片型号时。`mcu` 同时决定启动文件与链接脚本的自动匹配（如 `stm32g474ret6` → `STM32G474RETX_FLASH.ld`）。**同时**改 `.ioc` 的 `Mcu.CPN`、`Mcu.Name`、`Mcu.Package`、`ProjectManager.DeviceId`，并在 CubeMX 里重新生成。 |
| B3 | `boards/*.json` 的 `f_cpu` | `170000000L` | 仅影响部分工具链的默认宏（`F_CPU`），不改变 `.ioc` 里的真实时钟。跑 170 MHz 时保持此值；超频到 200 MHz 属板级实验，不在模板承诺范围内。 |
| B4 | `.vscode/tasks.json` 的 `windows.command` | `C:\ST\STM32CubeMX\STM32CubeMX.exe` 与同目录 `jre\bin\java.exe` | CubeMX **不在** `C:\ST\STM32CubeMX` 时。官方安装器默认装到 `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX\`，本机不是；定位方法见 `docs/TOOLING-cubemx.md` §1。 |
| B5 | `.vscode/tasks.json` 的 `linux.command` | `STM32CubeMX` | Linux 下要求该命令在 `PATH` 中。 |
| B6 | `.vscode/launch.json`、`.vscode/settings.json` 的终端/包路径 | `${env:USERPROFILE}/.platformio/...` | PlatformIO Core/包目录被 `PLATFORMIO_CORE_DIR` 改到别处时，同步调整调试路径和 `penv/Scripts` 路径（WSL 也要留意）。 |
| B7 | `platformio.ini` 的版本锁定 | `ststm32@20.0.0`、`framework-stm32cubeg4@1.6.3`、`toolchain-gccarmnoneeabi@1.70201.0`、`tool-ldscripts-ststm32@0.3.0`、`tool-openocd@3.1200.7` | 升级时**必须一起验证**"生成代码 + 构建"两者，不要单独升 HAL。 |
| B8 | `.ioc` 与 CubeMX 版本 | 本次由 6.18.1 / DB.6.0.181 生成，FW_G4 锁定 V1.6.3 | 实际工具版本看 exe 属性/运行日志，不要仅相信文件里的版本戳。迁移后审阅工具写回并验证生成 + 构建。 |

## C. 环境前提（不是文件，但缺了就跑不起来）

| # | 前提 | 检查方式 | 缺失后果 |
| - | ---- | -------- | -------- |
| C1 | STM32CubeMX 6.x 已安装 | 本机路径 `C:\ST\STM32CubeMX\STM32CubeMX.exe` | 无法改 `.ioc`、无法生成 `Core/` |
| C2 | 已安装 FW_G4 包（V1.6.3） | `C:\Users\Leon\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.3`；或 CubeMX → Help → Manage embedded software packages | `project generate` 失败 |
| C3 | PlatformIO Core | `pio --version` | 无法构建/烧录 |
| C4 | 主机 GCC（仅跑 `test/stdio_port_test.c` 需要） | `gcc --version` | 回归测试跑不了；`pio run` 不受影响 |
| C5 | 默认 ST-Link，使用 SWD；目标板正确供电、共地，VTref 接线按探针定义 | 看探针型号与接线 | 探针/固件不匹配或目标无电会连接失败 |
| C6 | 首次 `pio run` 需联网下载 ststm32 平台与 CubeG4 包 | — | 离线首次构建失败 |

## D. 已知陷阱（不改也会踩）

| # | 陷阱 | 现状 | 建议 |
| - | ---- | ---- | ---- |
| D1 | `.vscode/settings.json` 开了 `editor.formatOnSave: true` | 保存 `Core/Src/main.c` 会被 clang-format 整体重排，产生与 CubeMX 无关的巨大 diff | 处理 `Core/**` 时建议关掉 formatOnSave，或加 `.clang-format` + `// clang-format off` 保护 |
| D2 | `.ioc` 行尾 | CubeMX 写 **CRLF**；Git 入库/checkout 与编辑器统一 LF，`text` 属性在 add/比对时归一化 | 不必手工转回 CRLF；不要提交纯行尾变化，异常时用 `git ls-files --eol` 核对 |
| D3 | `.ioc` 内的参数白名单 | 每个外设有一行 `<IP>.IPParameters=a,b,c`，每个引脚有一行 `<Pin>.GPIOParameters=...`，只有列在这里的 key 才被识别 | 手改 `.ioc` 加参数时，**必须同步补进对应白名单行**，否则 CubeMX 静默忽略。本次 `PC13` 的 `PinState` 就是靠 `PC13.GPIOParameters=PinState` 生效的 |
| D4 | 信号名是 `S_` 前缀的枚举 | 例如 PA0 上 TIM2 通道 1 的**正确**写法是 `S_TIM2_CH1_ETR`，写成 `TIM2_CH1_ETR` 会失败 | 信号名一律以本机 `C:\ST\STM32CubeMX\db\mcu\STM32G474V(B-C-E)Tx.xml` 为准，先查库 |
| D5 | CubeMX 对 `.ioc` 的错误是**非致命**的 | 信号名写错时只在日志里留一行 `(OptionalMessage_ERROR) Cannot map signal ...`，`project generate` 依然返回成功，外设被静默丢弃 | 改过 `.ioc` 后**必须**核验：①日志有无 `Cannot map`；②生成的 `main.c` 里有没有对应的 `MX_xxx_Init` |
| D6 | `Drivers/` 被 git 忽略 | `/Drivers/**` + 逐级放行 `Drivers/CMSIS/`、`Drivers/STM32G4xx_HAL_Driver/` 及其 `.gitkeep` | 新克隆的仓库里 `Drivers/` 只有 `.gitkeep`，这是**预期**的：HAL/CMSIS 由 PlatformIO framework 提供。注意 git 不能 re-include 被排除目录下的文件，加新占位目录时要同时放行目录本身 |
| D7 | `ProjectManager.AskForMigrate` 被写回 `true` | 模板里提交为 `false`，但 CubeMX 每次生成/保存都会改回 `true` | 无头生成前若卡住，先把它改回 `false`；见 `docs/TOOLING-cubemx.md` §3 |
| D8 | G474 最小工程也会有一条 `VP_RIF_VS_RIF1` 诊断 | 即使一个外设都不加，`project generate` 仍打印 `(OptionalMessage_ERROR) Pin5 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU` | 与本工程引脚无关，属 CubeMX 6.18.1 的 G4 数据库噪音；只要没有 `Cannot map` 且 `MX_xxx_Init` 齐全即可，不必删外设去"修"它 |
| D9 | CubeMX 重复生成会把 `syscalls.c` / `sysmem.c` 写成叠加路径 | 日志报 `FileNotFoundException: <工程目录>\D:\...\Core\Src\syscalls.c`，这两个文件不会更新 | 这两份是标准的 newlib 桩，改动很少；若需要更新，从 CubeMX 的 `FW_G4` 模板目录复制，或先删除再生成 |

---

## E. 推荐的「改配置」流程（含校验）

按优先级：

1. **首选**：用 CubeMX GUI 改，保存后 `project generate`。
2. **脚本化/headless**：用 `scripts/cubemx-generate.mx`（加载已有 `.ioc` → 生成），
   工作目录必须是项目根目录。
3. **手改 `.ioc`**：仅限小改动，且**必须**做完 D5 的两项核验。
4. **有 MCP 就用 MCP**：本机全局装有 `vscode-cube-mcp`（见 `docs/TOOLING-cubemx.md`），
   提供 `cubemx_load`（只读回读）/ `cubemx_configure`（带备份写回）/ `cubemx_generate`，
   并强制 `.ioc` 路径白名单 —— 比裸手改安全。
