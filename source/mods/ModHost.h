#pragma once

// ============================================================================
// ModHost - SpeedyNote 插件宿主 (库 A: SpeedyNote-PluginHost)
// ============================================================================
// 这是唯一需要侵入上游的代码，且用 #ifdef SPEEDYNOTE_MODS 完全包起来。
// 关闭宏 = 原版 SpeedyNote，性能行为完全一致。
//
// 编译期静态注册：Mod 在编译时链接进来，通过 registerMod() 注册自己。
// 没有动态库加载、没有脚本引擎、没有运行时开销。
// 单个 Mod 抛异常会被宿主的 try/catch 吞掉，不影响主程序。
// ============================================================================

#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

namespace ModHostNS {

// Mod 生命周期接口
class IMod {
public:
    // 构造时自动注册，析构时自动注销
    IMod();
    virtual ~IMod();

    // Mod 唯一 id，如 "sync"
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;

    // 启动：主程序 QApplication 创建后调用
    virtual bool initialize(QObject* app) = 0;

    // 关闭：程序退出前调用
    virtual void shutdown() = 0;
};

// ModHost 单例
class ModHost : public QObject {
    Q_OBJECT

public:
    static ModHost* instance();

    // Mod 在静态初始化时调用此函数注册自己
    static void registerMod(IMod* mod);

    // 宿主调用：初始化所有已注册 Mod
    void initializeAll(QObject* app);

    // 宿主调用：关闭所有 Mod
    void shutdownAll();

    // 查询
    QVector<IMod*> mods() const { return m_mods; }

signals:
    void modInitialized(const QString& id, bool ok, const QString& message);

private:
    explicit ModHost(QObject* parent = nullptr);
    QVector<IMod*> m_mods;
    static ModHost* s_instance;
    static QVector<IMod*>& registry();
};

// 辅助宏：Mod 源码里写 REGISTER_MOD(MyModClass) 即可自动注册
// IMod 构造函数会自动把自己注册进宿主；这里只需创建一个全局静态实例。
// 用函数级静态（由静态对象构造触发）避开跨编译单元初始化顺序问题。
#define REGISTER_MOD(ClassName) \
    static ClassName ClassName##_instance

} // namespace ModHostNS
