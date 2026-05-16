# Wii-U-Pad-Macro（GamePad Macro Demo）

实验性 **Aroma WUPS** 插件：在 `VPADRead` 返回之后向 `VPADStatus` 合并**可配置的宏脚本**（SD 槽位、逗号分隔脚本、全屏节点图编辑；关闭配置菜单并延迟后执行）。

**上游仓库**：<https://github.com/wei134102/Wii-U-Pad-Macro>

---

## 思路与代码借鉴出处（Attribution）

本仓库**独立实现** Wii U 侧宏逻辑与脚本解析；下列项目提供了**设计思路**或**工程布局参考**，**并非**整体拷贝其业务源码：

| 来源 | 借鉴内容 |
|------|-----------|
| **[hid_to_vpad](https://github.com/Maschell/hid_to_vpad)**（Maschell） | `src/patcher/hid_controller_function_patcher.cpp` 一类思路：在 **`VPADRead` 返回之后** 改写 `VPADStatus` 缓冲（`hold` / `trigger` / `release` 等字段）。本插件使用 **WUPS `WUPS_MUST_REPLACE` + WUT `VPADRead`** 达到同层效果。 |
| **[controller_patcher](https://github.com/Maschell/controller_patcher)** | 旧 HBL 栈下 HID→`VPADData` 的生态背景说明；本仓库**不**链接 `libcontrollerpatcher`。 |
| **[Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync)** | **目录布局**：与本目录**同级**放置时，通过其 `external/libwupsxx` 与 `WiiUPluginSystem` 完成 WUPS 链接（与常见 WUPS 示例相同做法）。 |
| **devkitPro / [wut](https://github.com/devkitPro/wut) / WUPS** | Wii U 工具链、`VPAD` 类型与插件入口。 |

插件元数据中亦注明了与 `hid_to_vpad` Hook 模式的渊源（见 `source/main.cpp` 内 `WUPS_PLUGIN_*`）。

---

## 功能概要

- SD 卡多槽宏、`macro_script` 逗号脚本（含两段式 `第一段&延迟&第二段+hold` 等扩展）。
- Wii U 配置界面图编辑；PC 侧 `PC_Edieted/` 提供 Tk 节点编辑器与 `macros.ini` / `macroN.txt` 同步。

---

## 构建

1. 与 [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync) **同级目录**放置本仓库，以便 `Makefile` 解析 `../Wii-U-Time-Sync/external/libwupsxx/...`（见仓库根目录 `Makefile` 中 `PARENT_DIR`）。
2. 安装 **devkitPro**，配置 `DEVKITPRO` 环境变量。
3. 在仓库根目录执行：`make`  
   产物示例：`GamePad_Macro_Demo.wps`（具体以 `Makefile` 为准）。

---

## 使用（简要）

1. 安装生成的 `.wps` 插件。拷贝到sd:/wiiu/environments/aroma/plugins/下面
宏文件放到sd:/wiiu/gamepad_macro/下面
2. 打开 WUPS 配置 → 设置宏延迟、编辑槽位脚本或节点图(尽量使用PC端来编辑,wiiu端编辑费事)。
3. **关闭配置菜单** 后按延迟 **armed**，对 Player 1 GamePad 注入宏序列。
4.编辑生成新的宏可以通过FTP 上传到sd:/wiiu/gamepad_macro/下面，直接在游戏里面实验，不用关闭游戏和重启插件。
5.插件默认开启，就是每次通过按 -和L和方向下 一起按，进入armoa插件界面，然后按B退出就可以延迟触发

---

## 使用许可（License）

- **本仓库原创代码**：以 **MIT License** 发布（见仓库根目录 [`LICENSE`](LICENSE)）。各源文件中的 `SPDX-License-Identifier: MIT` 与此一致。
- **构建时链接的库**（如 `libwups`、`libwut`、WiiUPluginSystem 提供的导入库等）遵循 **devkitPro / 对应上游** 的分发条款；**不包含**在本文档内逐条复述时，请以你本机工具链与 [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync) 树内许可证文件为准。
- 若你**复制或分发** [hid_to_vpad](https://github.com/Maschell/hid_to_vpad) 等第三方**源码**，须遵守**该项目自身许可证**（与本仓库 MIT **无关**）；本仓库仅文档层面说明思路来源。

---

## PC 编辑器

见 `PC_Edieted/`（Python + Tk），用于编辑 `sd/wiiu/gamepad_macro` 下的 `macros.ini` 与 `macro1.txt` … `macro8.txt`。

- **界面语言**：窗口顶部 **界面语言** 下拉框可切换 **中文 / English**；选择会写入同目录的 `pc_gui_locale.txt`（打包成 exe 后则写在 exe 同目录）。这与 `macros.ini` 里的 **`language`（主机插件界面语言）** 是两项独立设置。
- **GitHub Actions**：推送 `main`/`master` 或 PR 会编译 **`.wps`** 与 **PC zip** 并上传 Artifact；推送 **`v*` 标签**（如 `v0.1.0`）时额外创建 **GitHub Release**，附带 `GamePad_Macro_Demo.wps` 与 `GamePadMacroEditor-windows.zip`。依赖上游仓库 [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync)（`master` + submodules），见 `.github/workflows/main.yml`。

---

## 免责声明

本软件按「原样」提供，不作任何明示或暗示担保。使用本插件修改手柄输入可能影响游戏行为，请自行承担风险。
