# SpeedyNote-PluginHost（库 A：插件版软件本体）

基于原作者 [alpha-liu-01/SpeedyNote](https://github.com/alpha-liu-01/SpeedyNote) 的**插件版本体**。

## 核心原则

1. **保持作者原思路**：内核代码（Document / Viewport / strokes / pdf）一字不改，保留"原生 C++、低内存、旧设备能跑"的特性。
2. **只保 Win + Android**：砍掉 macOS / Linux / iOS / HarmonyOS 的构建目标，减小体积和编译时间。
3. **编译期 Mod**：所有新功能（含同步）静态编译进来，无动态库加载、无脚本引擎、无运行时开销。
4. **最小侵入**：对上游的改动集中在一处（`Main.cpp` 加几行），用 `#ifdef SPEEDYNOTE_MODS` 完全包裹——关掉宏就是原版。

## 目录说明

```
mods/
  ModHost.h/.cpp          Mod 注册中心（静态注册，异常隔离）
  SyncModAdapter.h/.cpp   把库 B 的 SyncManager 包装成 Mod
patches/
  Main.cpp.patch          对上游 Main.cpp 的最小改动
  CMakeLists.patch        对上游 CMake 的改动
  APPLY.md                如何应用补丁（上游更新后重放）
```

## 对上游的改动（共 3 处，全部可关）

| 文件 | 改动 | 关闭方式 |
|---|---|---|
| `source/Main.cpp` | 加 `#include "mods/ModHost.h"`、启动时 `ModHost::initializeAll(&app)`、退出时 `shutdownAll()` | `#ifdef SPEEDYNOTE_MODS` |
| `CMakeLists.txt` | 加 `option(SPEEDYNOTE_MODS ...)`、引入库 B、追加 mods 源文件 | 选项 OFF |
| 无其他 | 作者核心代码零改动 | — |

## 构建

### Windows（MSYS2 clang64）
```bash
cmake -B build -DSPEEDYNOTE_MODS=ON -DSPEEDYNOTE_ENABLE_VISION_OCR=OFF
cmake --build build
```

### Android
通过上游的 Docker 构建脚本，加 `-DSPEEDYNOTE_MODS=ON`。

## 与库 B 的关系

库 B（`SpeedyNote-SyncMod`）是独立静态库，本库通过 `add_subdirectory` 在编译期引入。库 B 不依赖本库，本库通过 `SyncModAdapter` 单向使用库 B。两个库可以独立开发、独立测试。

## 上游更新后怎么办

见 `patches/APPLY.md`。核心思路：本库不保留 fork 的合并历史，而是维护一组可重放的补丁。上游更新后重新 apply，冲突只发生在那几行接入代码上。
