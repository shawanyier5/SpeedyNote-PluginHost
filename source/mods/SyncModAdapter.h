#pragma once

// ============================================================================
// SyncModAdapter - 把库 B（SyncMod）包装成库 A 可识别的 Mod
// ============================================================================
// 这是两个库之间的唯一粘合层。库 B 不依赖库 A，库 A 通过本适配器用库 B。
// 宿主只需连接几个信号即可完成同步，不碰库 B 内部实现。
// ============================================================================

#include "ModHost.h"

#include <QObject>

namespace SyncMod {
class SyncConfig;
class SyncManager;
}

class SyncModAdapter : public QObject, public ModHostNS::IMod {
    Q_OBJECT

public:
    explicit SyncModAdapter(QObject* parent = nullptr);
    ~SyncModAdapter() override;

    // IMod 接口
    QString id() const override { return QStringLiteral("sync"); }
    QString displayName() const override { return QStringLiteral("多端同步（WebDAV / OneDrive）"); }
    bool initialize(QObject* app) override;
    void shutdown() override;

private:
    SyncMod::SyncConfig* m_config = nullptr;
    SyncMod::SyncManager* m_manager = nullptr;
};
