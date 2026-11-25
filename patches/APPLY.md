# 如何应用补丁到上游 SpeedyNote

## 初始化（一次性）

```bash
# 目录结构
D:/GZ/SpeedyNote-Mods/
  SpeedyNote/              ← 上游源码（从 codeload 下载，保持纯净）
  SpeedyNote-PluginHost/   ← 库 A：本体补丁 + Mod 宿主
  SpeedyNote-SyncMod/      ← 库 B：同步模块

# 把库 A 的 mods 目录拷贝进上游源码
cp -r SpeedyNote-PluginHost/mods/* SpeedyNote/source/mods/
```

## 应用 Main.cpp 改动

打开 `SpeedyNote/source/Main.cpp`：

**改动 1** — 在文件头部 include 区（约第 11 行 `<QSettings>` 之后）加：
```cpp
#ifdef SPEEDYNOTE_MODS
#include "mods/ModHost.h"
#endif
```

**改动 2** — 在 `QApplication app(argc, argv);` 之后（约第 1224 行，`app.setApplicationName("App");` 之后）加：
```cpp
#ifdef SPEEDYNOTE_MODS
    ModHostNS::ModHost::instance()->initializeAll(&app);
#endif
```

**改动 3** — 在 `int exitCode = app.exec();` 之后、`return exitCode;` 之前（约第 1539 行）加：
```cpp
#ifdef SPEEDYNOTE_MODS
    ModHostNS::ModHost::instance()->shutdownAll();
#endif
```

## 应用 CMakeLists.txt 改动

**改动 1** — 在 `option(...)` 区（约第 14 行附近）加：
```cmake
option(SPEEDYNOTE_MODS "Enable compile-time mod host" ON)
```

**改动 2** — 在 `add_executable` 之前，链接库 B：
```cmake
if(SPEEDYNOTE_MODS)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../SpeedyNote-SyncMod ${CMAKE_BINARY_DIR}/syncmod)
    list(APPEND APP_SOURCES
        ${CMAKE_SOURCE_DIR}/source/mods/ModHost.cpp
        ${CMAKE_SOURCE_DIR}/source/mods/SyncModAdapter.cpp
    )
endif()
```

**改动 3** — 在 target 定义后加链接：
```cmake
if(SPEEDYNOTE_MODS)
    target_link_libraries(speedynote PRIVATE SpeedyNoteSyncMod)
    target_compile_definitions(speedynote PRIVATE SPEEDYNOTE_MODS)
endif()
```

## 上游更新后的重放

```bash
# 1. 重新下载上游源码覆盖 SpeedyNote/（保留 mods/ 目录）
# 2. 按上面的 5 处改动重新 apply
# 3. 行号会漂移，但搜索关键代码字符串定位即可
# 4. git diff 检查只有这 5 处改动
```

## 验证纯净性

```bash
cd SpeedyNote
git diff --stat
# 应该只显示 Main.cpp 和 CMakeLists.txt 两个文件
# source/core/、source/strokes/、source/pdf/ 等核心目录零改动
```
