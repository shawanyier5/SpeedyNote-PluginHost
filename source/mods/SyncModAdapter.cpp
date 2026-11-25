#include "SyncModAdapter.h"

#include "SyncConfig.h"
#include "SyncManager.h"

// 复用上游的 .snbx 导入器，保持和手动导入完全一致的解包/改名逻辑。
// 适配器位于 source/mods/，导入器位于 source/sharing/。
#include "../sharing/NotebookImporter.h"

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    // 默认本地笔记本目录：必须与 DocumentManager 实际使用的路径一致，
    // 否则 scanNotebooks 扫不到任何 .snb，同步会变成空转。
    if (m_config->localNotebookDir().isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_config->setLocalNotebookDir(dataDir + QStringLiteral("/notebooks"));
    }

    m_manager = new SyncMod::SyncManager(m_config, this);

    // 宿主职责：把远端 .snbx 真正导入 SpeedyNote 的笔记本目录。
    // 原则：绝不静默覆盖。本地若已有同名笔记本，先整体改名备份再导入；
    // 导入失败则回滚备份，保证用户数据不丢。
    connect(m_manager, &SyncMod::SyncManager::applyRemoteSnapshot, this,
            [this](const QString& remoteName, const QString& tmpPath,
                   const QString& etag) {
        const QString destDir = m_config->localNotebookDir();

        QString baseName = remoteName;
        if (baseName.endsWith(".snbx", Qt::CaseInsensitive))
            baseName.chop(5);
        const QString localSnb = QDir(destDir).filePath(baseName + ".snb");

        // 同名笔记本已存在 -> 先备份，避免静默覆盖
        QString backupPath;
        if (QFileInfo::exists(localSnb)) {
            const QString stamp =
                QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
            backupPath = localSnb + ".bak-" + stamp;
            if (!QFile::rename(localSnb, backupPath)) {
                qWarning() << "[SyncMod] backup failed, skip import:" << localSnb;
                QFile::remove(tmpPath);
                return;
            }
            qInfo() << "[SyncMod] local notebook backed up to:" << backupPath;
        }

        const NotebookImporter::ImportResult result =
            NotebookImporter::importPackage(tmpPath, destDir);

        if (!result.success) {
            // 导入失败：清掉可能的残留，恢复备份
            if (!backupPath.isEmpty()) {
                QDir(destDir).rmdir(QFileInfo(localSnb).fileName());
                QFile::rename(backupPath, localSnb);
            }
            qWarning() << "[SyncMod] import failed:" << result.errorMessage;
        } else {
            m_config->setEtag(remoteName, etag);
            qInfo() << "[SyncMod] imported:" << result.extractedSnbPath
                    << (backupPath.isEmpty() ? QString() :
                        QStringLiteral(" (local backup: %1)").arg(backupPath));
        }

        QFile::remove(tmpPath);   // 临时 .snbx 用完即删
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
