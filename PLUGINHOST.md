# SpeedyNote-PluginHost（库 A：插件版软件本体）

基于原作者 [alpha-liu-01/SpeedyNote](https://github.com/alpha-liu-01/SpeedyNote) 的**插件版本体**。

## 核心原则

1. **保持作者原思路**：内核代码（Document / Viewport / strokes / pdf）一字不改，保留"原生 C++、低内存、旧设备能跑"的特性。
2. **只用 Win + Android**：当前只验证这两个平台（旧设备、低内存优先）；上游其他平台代码原样保留，不主动维护。
3. **编译期 Mod**：所有新功能（含同步）静态编译进来，无动态库加载、无脚本引擎、无运行时开销。
4. **最小侵入**：对上游的改动只有 3 个文件、每处几行，全部用 `#ifdef SPEEDYNOTE_MODS` 完全包裹——关掉宏就是原版。

## 目录说明

```
mods/
  ModHost.h/.cpp          Mod 注册中心（静态注册，异常隔离）
  SyncModAdapter.h/.cpp   把库 B 的 SyncManager 包装成 Mod，负责 .snbx 导入与冲突处理
  SyncSettingsUI.h/.cpp   设置面板"同步"标签页 + 冲突选择对话框
patches/
  Main.cpp.patch          对上游 Main.cpp 的最小改动
  CMakeLists.patch        对上游 CMake 的改动
  APPLY.md                如何应用补丁（上游更新后重放）
```

## 对上游的改动（共 3 处，全部可关）

| 文件 | 改动 | 关闭方式 |
|---|---|---|
| `source/Main.cpp` | 加 `#include "mods/ModHost.h"`、启动时 `ModHost::initializeAll(&app)`、退出时 `shutdownAll()` | `#ifdef SPEEDYNOTE_MODS` |
| `source/ControlPanelDialog.cpp` | 加 `#include "SyncSettingsUI.h"`、构造函数末尾调用 `addSyncTab()` | `#ifdef SPEEDYNOTE_MODS` |
| `CMakeLists.txt` | 加 `option(SPEEDYNOTE_MODS ...)`、引入库 B、追加 mods 源文件 | 选项 OFF |
| 无其他 | 作者核心代码（Document / Viewport / strokes / pdf）零改动 | — |

## 构建

### Windows（MSYS2 clang64）
```bash
cmake -B build -DSPEEDYNOTE_MODS=ON \
      -DSPEEDYNOTE_ENABLE_WINDOWS_INK_OCR=OFF \
      -DQT_PATH=<你的 msys64>/clang64
cmake --build build
```
（`SPEEDYNOTE_ENABLE_WINDOWS_INK_OCR=OFF` 用于没有 Windows SDK / C++WinRT 的环境；有 SDK 时可以开。）

### Android
通过上游的 Docker 构建脚本，加 `-DSPEEDYNOTE_MODS=ON`。

## 同步功能怎么用

**入口（两种，任选）**：
1. 主界面 **「...」菜单 →「立即同步」/「同步设置...」**（推荐， easiest）
2. 设置 → **同步** 标签页（标签较多，可能需要滚动标签栏）

设置页内容：

- **同步平台**：禁用 / WebDAV（坚果云等支持 WebDAV 的网盘）/ OneDrive 本地文件夹。
- **同步间隔**：0 = 只手动同步；否则每 N 分钟一次（1–1440）。
- **启动行为**：可勾选"软件启动时自动同步一次"。
- **笔记本目录**：默认跟随 SpeedyNote 的 `AppDataLocation/notebooks`，一般不用改。
- WebDAV 填服务器地址、用户名、密码；OneDrive 只需选本地同步文件夹（不接 Graph API，免 OAuth）。
- **测试连接 / 立即同步** 两个按钮可在保存设置后直接验证。

同步链路是真正的 `.snbx` 双向同步：本地 `.snb` 打包成 ZIP 上传；远端包解包后用上游 `NotebookImporter` 导入。每本书的上次同步 ETag 会记下来，远端没变化时跳过，避免反复导入/备份。

**PDF 一起同步**：打包时会读取 `document.json`（兼容 `pdf_path` / `pdf_relative_path` / 多 PDF 的 `pdf_sources[]`），把笔记本外部引用的 PDF 一并放进 zip 的 `embedded/`，并改写 JSON 里的路径为 `../embedded/xxx.pdf`——和上游导出格式一致，所以 PDF 批注笔记本换设备也能正常打开。

**冲突绝不静默覆盖**：本地与远端同名时弹四选一对话框——保留本地 / 使用远端（保留备份后覆盖）/ 两份都留（推荐）/ 取消。选"两份都留"时本地原版改名为 `xxx.snb.bak-时间戳` 保留。

## 与库 B 的关系

库 B（`SpeedyNote-SyncMod`）是独立静态库，本库通过 `add_subdirectory` 在编译期引入。库 B 不依赖本库，本库通过 `SyncModAdapter` 单向使用库 B。两个库可以独立开发、独立测试。

## 上游更新后怎么办

见 `patches/APPLY.md`。核心思路：本库不保留 fork 的合并历史，而是维护一组可重放的补丁。上游更新后重新 apply，冲突只发生在那几行接入代码上。
