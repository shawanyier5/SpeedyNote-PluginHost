#include "ModHost.h"

#include <QDebug>

namespace ModHostNS {

ModHost* ModHost::s_instance = nullptr;

QVector<IMod*>& ModHost::registry()
{
    static QVector<IMod*> mods;
    return mods;
}

IMod::IMod()
{
    ModHost::registerMod(this);
}

IMod::~IMod()
{
    QVector<IMod*>& mods = ModHost::registry();
    mods.removeAll(this);
}

ModHost::ModHost(QObject* parent)
    : QObject(parent)
{
}

ModHost* ModHost::instance()
{
    if (!s_instance)
        s_instance = new ModHost();
    return s_instance;
}

void ModHost::registerMod(IMod* mod)
{
    if (!mod)
        return;
    registry().append(mod);
}

void ModHost::initializeAll(QObject* app)
{
    qInfo() << "[ModHost] initializing" << registry().size() << "mod(s)";
    for (IMod* mod : registry()) {
        bool ok = false;
        QString message;
        try {
            ok = mod->initialize(app);
            if (ok) {
                m_mods.append(mod);
                qInfo() << "[ModHost] mod loaded:" << mod->id();
            } else {
                message = QStringLiteral("initialize returned false");
                qWarning() << "[ModHost] mod failed:" << mod->id() << message;
            }
        } catch (const std::exception& e) {
            message = QString::fromUtf8(e.what());
            qWarning() << "[ModHost] mod threw:" << mod->id() << message;
        } catch (...) {
            message = QStringLiteral("unknown exception");
            qWarning() << "[ModHost] mod threw unknown:" << mod->id();
        }
        emit modInitialized(mod->id(), ok, message);
    }
}

void ModHost::shutdownAll()
{
    // 逆序关闭
    for (auto it = m_mods.rbegin(); it != m_mods.rend(); ++it) {
        try {
            (*it)->shutdown();
        } catch (...) {
            qWarning() << "[ModHost] mod shutdown threw:" << (*it)->id();
        }
    }
    m_mods.clear();
}

} // namespace ModHostNS
