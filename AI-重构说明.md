# AI 重构说明 / What Was Changed by AI

> 本文件由 AI 生成，用于说明本派生版本相对原作者项目做了哪些改动。

## 出处

| 项目 | 说明 |
|---|---|
| 原始项目 | [alpha-liu-01/SpeedyNote](https://github.com/alpha-liu-01/SpeedyNote) |
| 原作者 | alpha-liu-01 |
| 许可证 | GPL-3.0（版权归原作者，本派生版沿用） |
| 派生版本 | SpeedyNote-PluginHost（本地版号 `SpeedyNote插件版 1.0`） |
| 改造者 | **AI（WorkBuddy）辅助重构**，非原作者维护 |
| 配套仓库 | `SpeedyNote-SyncMod`（同步模块静态库） |

## 核心原则

1. 作者内核代码 **零改动** —— `source/core/`（Document / Viewport / strokes / pdf）等一行未动。
2. 所有改动用 `#ifdef SPEEDYNOTE_MODS` 编译期开关包裹；**关掉宏就是原版行为**。
3. 保持原生 C++ / Qt 的性能与低内存特性，无脚本引擎、无运行时动态库加载。
4. 只维护 Windows + Android 两个目标（用户实际使用场景）。

## 一、对上游 4 个文件的改动（均可一键关闭）

| 文件 | 改动 | 性质 |
|---|---|---|
| `source/Main.cpp` | 加 ModHost 的 include、启动 `initializeAll`、退出 `shutdownAll`（3 处） | 纯接入，不改逻辑 |
| `source/MainWindow.cpp` | 溢出菜单加「立即同步」/「同步设置...」两个入口 | 新增 UI 入口 |
| `source/ControlPanelDialog.cpp/.h` | 设置面板注入「同步」标签页；加 `switchToSyncTab()` | 新增 UI 页 |
| `CMakeLists.txt` | 加 `SPEEDYNOTE_MODS` 开关、引入同步库、追加源码、修正 Win Ink OCR 强制开启逻辑 | 构建配置 |

逐行补丁与重放方法见 `patches/APPLY.md`。

## 二、新增的 Mod 框架（本仓库 `source/mods/`）

| 文件 | 作用 |
|---|---|
| `ModHost.h/.cpp` | 编译期静态 Mod 注册中心：接口 `IMod`、静态注册宏、生命周期管理、异常隔离 |
| `SyncModAdapter.h/.cpp` | 把同步库包装成 Mod；负责 `.snbx` 导入、同名冲突备份、ETag 回填 |
| `SyncSettingsUI.h/.cpp` | 设置面板「同步」页 + 冲突四选一对话框 + 菜单入口查找 |

## 三、同步模块（另一个仓库 `SpeedyNote-SyncMod`，由本库编译期链接）

| 文件 | 作用 |
|---|---|
| `SyncConfig` | 平台选择（WebDAV / OneDrive 本地文件夹）、每 N 分钟间隔、凭据、ETag 持久化 |
| `SyncManager` | `VeryCoarseTimer` 定时调度、双向同步流程、冲突检测 |
| `WebDavBackend` | PROPFIND / GET / PUT / DELETE + Basic Auth + ETag |
| `OneDriveBackend` | 直接读写 OneDrive 本地同步文件夹（不走 Graph API，免 OAuth） |
| `NotebookPackage` | `.snb` 文件夹 ⇄ `.snbx` 内存 ZIP；**含外部 PDF 打包**（`embedded/`）与多 PDF 源兼容 |

## 四、功能层面的变化（用户可见）

- 真正双向同步：本地 `.snb` 打包上传、远端 `.snbx` 解包后用上游 `NotebookImporter` 导入。
- PDF 一起同步：外部 PDF 塞进 `embedded/`，改 JSON 路径为 `../embedded/xxx.pdf`。
- ETag 去重：远端没变化就跳过，避免重复导入/副本爆炸。
- 冲突不静默覆盖：四选一（保留本地 / 使用远端 / 两份都留 / 取消），同名本地先备份为 `.bak-时间戳`。
- 设置面板 + 主菜单双入口，可设同步间隔、平台、测试连接、立即同步。

## 五、性能优化取向

- 编译期静态链接，无动态加载开销；`QTimer::VeryCoarseTimer` 分钟级省电。
- 打包直接读文件流转 ZIP，不加载 `Document` 到内存。
- 保留作者原有 `-O3 -march=nehalem -msse4.2` 等针对旧设备的编译优化。
- 实测启动内存约 79–92 MB。

## 六、明确声明

- 本派生版本由 **AI（WorkBuddy）辅助重构**，不是原作者 alpha-liu-01 的作品或授权分支。
- 遇到问题请联系本仓库维护者，**不要打扰原作者**。
- 原作者 GPL-3.0 版权与署名始终保留（见各源文件头与上游 LICENSE）。
