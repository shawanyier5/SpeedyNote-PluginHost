#include "SyncModAdapter.h"

#include "SyncConfig.h"
#include "SyncManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

SyncModAdapter::SyncModAdapter(QObject* parent)
    : QObject(parent)
{
}

SyncModAdapter::~SyncModAdapter() = default;

bool SyncModAdapter::initialize(QObject* app)
{
    if (!app)
        return false;

    m_config = new SyncMod::SyncConfig(this);
    m_config->load();

    // 默认本地笔记本目录：跟随 SpeedyNote 的数据目录
    if (m_config->localNotebookDir().isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_config->setLocalNotebookDir(dataDir);
    }

    m_manager = new SyncMod::SyncManager(m_config, this);

    // 宿主职责：把 .snbx 数据交给 SpeedyNote 导入
    // 这里先留日志占位，实际导入逻辑需要接 NotebookImporter
    connect(m_manager, &SyncMod::SyncManager::applyRemoteSnapshot, this,
            [](const QString& path, const QByteArray& data) {
                qInfo() << "[SyncMod] received remote snapshot:" << path
                        << "(" << data.size() << " bytes)";
                // TODO: 调用 NotebookImporter 导入 data
            });

    connect(m_manager, &SyncMod::SyncManager::syncFinished, this,
            [](bool ok, const QString& msg) {
                qInfo() << "[SyncMod] sync" << (ok ? "OK" : "FAIL") << msg;
            });

    connect(m_manager, &SyncMod::SyncManager::conflictsDetected, this,
            [](const QList<SyncMod::Conflict>& conflicts) {
                qWarning() << "[SyncMod]" << conflicts.size() << "conflict(s) detected";
                // TODO: 弹出冲突选择对话框
            });

    m_manager->start();
    qInfo() << "[SyncMod] adapter initialized, platform ="
            << static_cast<int>(m_config->platform())
            << "interval =" << m_config->intervalMinutes() << "min";
    return true;
}

void SyncModAdapter::shutdown()
{
    if (m_manager) {
        m_manager->stop();
        m_manager->deleteLater();
        m_manager = nullptr;
    }
    if (m_config) {
        m_config->save();
        m_config->deleteLater();
        m_config = nullptr;
    }
}

// 静态注册：Mod 编译时自动进宿主注册表
// Q_OBJECT 在头文件里，AUTOMOC 会处理头文件，无需 .moc include
REGISTER_MOD(SyncModAdapter)
