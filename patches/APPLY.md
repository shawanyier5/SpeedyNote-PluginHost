# 如何应用补丁到上游 SpeedyNote

本文件用于**上游更新后重放改动**。当前本体已经是可编译状态，正常开发不需要执行下面的步骤。

## 目录结构

```
D:/GZ/SpeedyNote-Mods/
  SpeedyNote/              ← 上游源码 + 3 个文件的 Mod 接入改动（本仓库）
  SpeedyNote-PluginHost/   ← 库 A 说明文档 + mods/ 源码副本
  SpeedyNote-SyncMod/      ← 库 B：同步模块静态库
```

`SpeedyNote/source/mods/` 是**编译真相源**；`SpeedyNote-PluginHost/mods/` 是文档副本。
修改 Mod 代码时请改前者，再同步到后者，避免两边不一致导致编译到旧代码。

## 步骤 1：拷贝 Mod 源码

```bash
cp -r SpeedyNote-PluginHost/mods/. SpeedyNote/source/mods/
```

## 步骤 2：Main.cpp（3 处）

**改动 1** — include 区（约第 11 行 `<QSettings>` 之后）：
```cpp
#ifdef SPEEDYNOTE_MODS
#include "mods/ModHost.h"
#endif
```

**改动 2** — `QApplication app(argc, argv);` 及 `app.setApplicationName("App");` 之后：
```cpp
#ifdef SPEEDYNOTE_MODS
    ModHostNS::ModHost::instance()->initializeAll(&app);
#endif
```

**改动 3** — `int exitCode = app.exec();` 之后、`return exitCode;` 之前：
```cpp
#ifdef SPEEDYNOTE_MODS
    ModHostNS::ModHost::instance()->shutdownAll();
#endif
```

## 步骤 3：ControlPanelDialog.cpp（2 处）

**改动 1** — 顶部 include 区（三个 `#include "..."` 之后）：
```cpp
#ifdef SPEEDYNOTE_MODS
// PluginHost: 编译期 Mod 注入的同步设置页；关闭 Mod 时这段代码不存在
#include "SyncSettingsUI.h"
#endif
```

**改动 2** — 构造函数里 `createCacheTab(); createAboutTab();` 之后、按钮创建之前：
```cpp
#ifdef SPEEDYNOTE_MODS
    // PluginHost: 只在编译期 Mod 打开时增加"同步"设置页
    SyncModUI::addSyncTab(tabWidget, this);
#endif
```

**改动 3** — `switchToKeyboardShortcutsTab()` 之前加一个跳转方法（给主菜单"同步设置"用）：
```cpp
#ifdef SPEEDYNOTE_MODS
void ControlPanelDialog::switchToSyncTab()
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->tabText(i) == QStringLiteral("同步")) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }
}
#endif
```
ControlPanelDialog.h 里对应加声明（同样用 `#ifdef SPEEDYNOTE_MODS` 包裹）。

## 步骤 3.5：MainWindow.cpp（2 处）

**改动 1** — include 区（`ControlPanelDialog.h` 之后）：
```cpp
#ifdef SPEEDYNOTE_MODS
#include "SyncSettingsUI.h"
#include "SyncModAdapter.h"
#include "SyncManager.h"
#endif
```

**改动 2** — `overflowMenu` 里、`Settings` 菜单项之前插入：
```cpp
#ifdef SPEEDYNOTE_MODS
    QAction *syncNowAction = overflowMenu->addAction(tr("立即同步"));
    connect(syncNowAction, &QAction::triggered, this, [this]() {
        SyncModAdapter* adapter = SyncModUI::findAdapterForMenu();
        if (adapter && adapter->manager()) adapter->manager()->syncNow();
    });
    QAction *syncSettingsAction = overflowMenu->addAction(tr("同步设置..."));
    connect(syncSettingsAction, &QAction::triggered, this, [this]() {
        ControlPanelDialog dialog(this, this);
        dialog.switchToSyncTab();
        dialog.exec();
    });
    overflowMenu->addSeparator();
#endif
```

## 步骤 4：CMakeLists.txt

**改动 1** — option 区加开关：
```cmake
option(SPEEDYNOTE_MODS "Enable compile-time mod host (SpeedyNote-PluginHost)" OFF)
```

**改动 2** — Windows Ink OCR 自动开启逻辑加一个守卫，让显式 `-DSPEEDYNOTE_ENABLE_WINDOWS_INK_OCR=OFF` 生效（否则没有 WinSDK 的环境会硬编译失败）：
```cmake
if(NOT DEFINED SPEEDYNOTE_ENABLE_WINDOWS_INK_OCR AND NOT USE_QT5 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SPEEDYNOTE_ENABLE_WINDOWS_INK_OCR ON CACHE BOOL "")
elseif(SPEEDYNOTE_ENABLE_WINDOWS_INK_OCR)
    ...
else()
    message(STATUS "Windows Ink OCR: DISABLED")
endif()
```

**改动 3** — Mod 块（放在 target 定义之后、链接区之前）：
```cmake
if(SPEEDYNOTE_MODS)
    set(MODHOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source/mods")
    set(SYNCMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SpeedyNote-SyncMod")
    set(SYNCMOD_MINIZ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/miniz" CACHE PATH "" FORCE)
    add_subdirectory("${SYNCMOD_DIR}" "${CMAKE_BINARY_DIR}/syncmod")

    target_sources(speedynote PRIVATE
        "${MODHOST_DIR}/ModHost.cpp"
        "${MODHOST_DIR}/SyncModAdapter.cpp"
        "${MODHOST_DIR}/SyncSettingsUI.cpp"
    )
    target_include_directories(speedynote PRIVATE "${MODHOST_DIR}")
    target_compile_definitions(speedynote PRIVATE SPEEDYNOTE_MODS)
endif()
```

**改动 4** — 文件末尾追加链接（**必须用 plain 签名**，和 Windows 分支首个 `target_link_libraries` 保持一致，否则 CMake 报签名冲突）：
```cmake
if(SPEEDYNOTE_MODS)
    target_link_libraries(speedynote SpeedyNoteSyncMod)
endif()
```

## 上游更新后重放

```bash
# 1. 重新下载上游源码覆盖 SpeedyNote/（保留 source/mods/ 目录）
# 2. 重做上面 4 步；行号会漂移，用关键代码字符串定位
# 3. git diff --stat 检查只有 4 个文件：
#      source/Main.cpp
#      source/MainWindow.cpp
#      source/ControlPanelDialog.cpp + .h
#      CMakeLists.txt
# 4. source/core/、source/strokes/、source/pdf/ 等核心目录必须零改动
# 5. 重新 configure + build 验证
```

## 验证纯净性

```bash
cd SpeedyNote
git diff --stat
# 应只显示 Main.cpp、MainWindow.cpp、ControlPanelDialog.cpp/.h、CMakeLists.txt
```
