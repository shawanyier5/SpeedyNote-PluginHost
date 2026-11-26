#include "SyncSettingsUI.h"

#include "ModHost.h"
#include "SyncModAdapter.h"
#include "SyncConfig.h"
#include "SyncManager.h"
#include "SyncBackend.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <functional>

namespace SyncModUI {

// 供主界面菜单调用（外部链接）
SyncModAdapter* findAdapterForMenu()
{
    for (ModHostNS::IMod* mod : ModHostNS::ModHost::instance()->mods()) {
        if (mod && mod->id() == QStringLiteral("sync"))
            return dynamic_cast<SyncModAdapter*>(mod);
    }
    return nullptr;
}

namespace {

// 文件内的简写
static SyncModAdapter* findAdapter()
{
    return findAdapterForMenu();
}

// 目录选择行：输入框 + 浏览按钮
QWidget* makeDirRow(QWidget* parent, QLineEdit* edit,
                    const QString& title, const std::function<void()>& onPicked)
{
    QWidget* row = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit);
    QPushButton* browse = new QPushButton(QStringLiteral("浏览..."), row);
    layout->addWidget(browse);
    QObject::connect(browse, &QPushButton::clicked, row, [parent, edit, title, onPicked]() {
        const QString dir = QFileDialog::getExistingDirectory(parent, title, edit->text());
        if (!dir.isEmpty()) {
            edit->setText(dir);
            onPicked();
        }
    });
    return row;
}

} // namespace

void addSyncTab(QTabWidget* tabWidget, QWidget* parent)
{
    if (!tabWidget)
        return;

    QWidget* tab = new QWidget(parent ? parent : tabWidget);
    QFormLayout* form = new QFormLayout(tab);

    SyncModAdapter* adapter = findAdapter();
    if (!adapter || !adapter->config()) {
        form->addRow(new QLabel(QStringLiteral("同步模块未启用。请确认编译时开启了 SPEEDYNOTE_MODS。"), tab));
        tabWidget->addTab(tab, QStringLiteral("同步"));
        return;
    }

    SyncMod::SyncConfig* cfg = adapter->config();

    // ---- 控件 ----
    QComboBox* platformCombo = new QComboBox(tab);
    platformCombo->addItem(QStringLiteral("禁用（不同步）"), 0);
    platformCombo->addItem(QStringLiteral("WebDAV（坚果云等网盘）"), 1);
    platformCombo->addItem(QStringLiteral("OneDrive 本地文件夹"), 2);
    platformCombo->setCurrentIndex(static_cast<int>(cfg->platform()));

    QSpinBox* intervalSpin = new QSpinBox(tab);
    intervalSpin->setRange(0, 1440);
    intervalSpin->setSuffix(QStringLiteral(" 分钟"));
    intervalSpin->setSpecialValueText(QStringLiteral("0 = 仅手动同步"));
    intervalSpin->setValue(cfg->intervalMinutes());

    QCheckBox* startupCheck = new QCheckBox(QStringLiteral("软件启动时自动同步一次"), tab);
    startupCheck->setChecked(cfg->syncOnStartup());

    QLineEdit* dirEdit = new QLineEdit(cfg->localNotebookDir(), tab);
    dirEdit->setPlaceholderText(QStringLiteral("本机存放 .snb 笔记本的目录"));

    QLineEdit* urlEdit = new QLineEdit(cfg->webdavUrl(), tab);
    urlEdit->setPlaceholderText(QStringLiteral("https://dav.example.com/"));
    QLineEdit* userEdit = new QLineEdit(cfg->webdavUsername(), tab);
    QLineEdit* passEdit = new QLineEdit(cfg->webdavPassword(), tab);
    passEdit->setEchoMode(QLineEdit::Password);

    QLineEdit* oneEdit = new QLineEdit(cfg->onedriveLocalPath(), tab);
    oneEdit->setPlaceholderText(QStringLiteral("OneDrive 同步文件夹的本地路径"));

    QPushButton* testButton = new QPushButton(QStringLiteral("测试连接"), tab);
    QPushButton* syncButton = new QPushButton(QStringLiteral("立即同步"), tab);
    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(testButton);
    buttonRow->addWidget(syncButton);
    buttonRow->addStretch();

    QLabel* statusLabel = new QLabel(QStringLiteral("尚未同步"), tab);
    statusLabel->setWordWrap(true);

    // ---- 保存：任何改动都立即写盘并让 Manager 重建后端/定时器 ----
    auto save = [adapter, cfg, platformCombo, intervalSpin, startupCheck,
                 dirEdit, urlEdit, userEdit, passEdit, oneEdit]() {
        cfg->setPlatform(static_cast<SyncMod::SyncConfig::Platform>(
            platformCombo->currentData().toInt()));
        cfg->setIntervalMinutes(intervalSpin->value());
        cfg->setSyncOnStartup(startupCheck->isChecked());
        cfg->setLocalNotebookDir(dirEdit->text().trimmed());
        cfg->setWebdavUrl(urlEdit->text().trimmed());
        cfg->setWebdavUsername(userEdit->text().trimmed());
        cfg->setWebdavPassword(passEdit->text());
        cfg->setOnedriveLocalPath(oneEdit->text().trimmed());
        cfg->save();
        adapter->reloadSettings();
    };

    QObject::connect(platformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     tab, [save](int) { save(); });
    QObject::connect(intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     tab, [save](int) { save(); });
    QObject::connect(startupCheck, &QCheckBox::toggled, tab, [save](bool) { save(); });
    QObject::connect(dirEdit, &QLineEdit::editingFinished, tab, [save]() { save(); });
    QObject::connect(urlEdit, &QLineEdit::editingFinished, tab, [save]() { save(); });
    QObject::connect(userEdit, &QLineEdit::editingFinished, tab, [save]() { save(); });
    QObject::connect(passEdit, &QLineEdit::editingFinished, tab, [save]() { save(); });
    QObject::connect(oneEdit, &QLineEdit::editingFinished, tab, [save]() { save(); });

    // ---- 测试连接 / 立即同步 ----
    QObject::connect(testButton, &QPushButton::clicked, tab, [adapter, statusLabel]() {
        SyncMod::SyncManager* manager = adapter->manager();
        SyncMod::SyncBackend* backend = manager ? manager->currentBackend() : nullptr;
        if (!backend) {
            statusLabel->setText(QStringLiteral("请先在上方选择同步平台并保存设置"));
            return;
        }
        statusLabel->setText(QStringLiteral("正在测试连接..."));
        QObject::connect(backend, &SyncMod::SyncBackend::connectionTested, statusLabel,
                         [statusLabel](bool ok, const QString& msg) {
                             statusLabel->setText(ok ? QStringLiteral("连接正常：") + msg
                                                     : QStringLiteral("连接失败：") + msg);
                         },
                         Qt::SingleShotConnection);
        backend->testConnection();
    });

    QObject::connect(syncButton, &QPushButton::clicked, tab, [adapter, statusLabel]() {
        SyncMod::SyncManager* manager = adapter->manager();
        if (!manager) {
            statusLabel->setText(QStringLiteral("同步模块未就绪"));
            return;
        }
        statusLabel->setText(QStringLiteral("正在同步..."));
        manager->syncNow();
    });

    if (adapter->manager()) {
        QObject::connect(adapter->manager(), &SyncMod::SyncManager::syncFinished, tab,
                         [statusLabel](bool ok, const QString& msg) {
                             statusLabel->setText(ok ? QStringLiteral("同步完成：") + msg
                                                     : QStringLiteral("同步失败：") + msg);
                         });
    }

    form->addRow(QStringLiteral("同步平台"), platformCombo);
    form->addRow(QStringLiteral("同步间隔"), intervalSpin);
    form->addRow(QStringLiteral("启动行为"), startupCheck);
    form->addRow(QStringLiteral("笔记本目录"),
                 makeDirRow(tab, dirEdit, QStringLiteral("选择笔记本目录"), [save]() { save(); }));
    form->addRow(new QLabel(QStringLiteral("—— WebDAV 设置 ——"), tab));
    form->addRow(QStringLiteral("服务器地址"), urlEdit);
    form->addRow(QStringLiteral("用户名"), userEdit);
    form->addRow(QStringLiteral("密码"), passEdit);
    form->addRow(new QLabel(QStringLiteral("—— OneDrive 设置 ——"), tab));
    form->addRow(QStringLiteral("本地文件夹"),
                 makeDirRow(tab, oneEdit, QStringLiteral("选择 OneDrive 同步文件夹"), [save]() { save(); }));
    form->addRow(buttonRow);
    form->addRow(QStringLiteral("状态"), statusLabel);
    form->addRow(new QLabel(
        QStringLiteral("提示：密码保存在本机配置中。检测到同名笔记本时不会静默覆盖，会弹出冲突选择。"), tab));

    tabWidget->addTab(tab, QStringLiteral("同步"));
}

ConflictChoice askConflict(const QString& notebookName,
                           const QString& localPath,
                           QWidget* parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QStringLiteral("同步冲突"));
    dlg.setModal(true);

    auto* layout = new QVBoxLayout(&dlg);
    auto* info = new QLabel(
        QStringLiteral("笔记本「%1」在本地和远端都被修改了。\n\n本地位置：%2\n\n"
                       "请选择处理方式（任何选择都不会直接删除数据）：")
            .arg(notebookName, localPath), &dlg);
    info->setWordWrap(true);
    layout->addWidget(info);

    ConflictChoice result = ConflictChoice::Cancel;
    auto* buttonBox = new QDialogButtonBox(&dlg);
    QPushButton* keepLocal = buttonBox->addButton(QStringLiteral("保留本地\n（跳过远端版本）"),
                                                  QDialogButtonBox::ActionRole);
    QPushButton* useRemote = buttonBox->addButton(QStringLiteral("使用远端\n（保留备份后覆盖）"),
                                                  QDialogButtonBox::ActionRole);
    QPushButton* keepBoth = buttonBox->addButton(QStringLiteral("两份都留\n（推荐）"),
                                                 QDialogButtonBox::ActionRole);
    QPushButton* cancel = buttonBox->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    QObject::connect(keepLocal, &QPushButton::clicked, &dlg,
                     [&dlg, &result]() { result = ConflictChoice::KeepLocal; dlg.accept(); });
    QObject::connect(useRemote, &QPushButton::clicked, &dlg,
                     [&dlg, &result]() { result = ConflictChoice::UseRemote; dlg.accept(); });
    QObject::connect(keepBoth, &QPushButton::clicked, &dlg,
                     [&dlg, &result]() { result = ConflictChoice::KeepBoth; dlg.accept(); });
    QObject::connect(cancel, &QPushButton::clicked, &dlg,
                     [&dlg, &result]() { result = ConflictChoice::Cancel; dlg.reject(); });

    dlg.exec();
    return result;
}

} // namespace SyncModUI
