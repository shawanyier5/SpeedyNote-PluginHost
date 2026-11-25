#pragma once

// ============================================================================
// SyncSettingsUI - 同步 Mod 的设置面板与冲突对话框 (库 A / 编译期 Mod)
// ============================================================================
// 只在 SPEEDYNOTE_MODS 打开时参与编译；原版 SpeedyNote 行为完全不变。
// 提供两件事：
//   1. addSyncTab()  —— 往上游设置面板注入"同步"标签页
//   2. askConflict() —— 本地/远端同名笔记本时的四选一冲突对话框
// ============================================================================

#include <QString>

class QTabWidget;
class QWidget;

namespace SyncModUI {

// 往设置面板追加"同步"页；Mod 未初始化时也能安全调用
void addSyncTab(QTabWidget* tabWidget, QWidget* parent = nullptr);

enum class ConflictChoice {
    KeepLocal = 0,   // 保留本地，跳过本次远端版本
    UseRemote = 1,   // 使用远端，覆盖本地（导入成功后删除备份）
    KeepBoth  = 2,   // 两份都留（导入远端，本地改名备份）
    Cancel    = 3    // 取消，稍后再处理
};

// 模态冲突对话框，返回用户的选择
ConflictChoice askConflict(const QString& notebookName,
                           const QString& localPath,
                           QWidget* parent = nullptr);

} // namespace SyncModUI
