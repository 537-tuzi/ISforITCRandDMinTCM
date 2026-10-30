#include <QApplication>
#include <QFile>
#include <QFont>
#include "util/winmanager.h"
#include "util/UiScale.h"
#include "util/AppDefs.h"

static void loadStyle(QApplication& app)
{
    // 优先用编译进 exe 的资源（qrc），任何工作目录都生效
    QStringList candidates;
    candidates << QStringLiteral(":/style.qss")
               << QCoreApplication::applicationDirPath() + "/style.qss"
               << QStringLiteral("style.qss");
    foreach (const QString& p, candidates) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 交给 UiScale 管理：窗口放大时按比例重建全局样式表
            UiScale::init(&app, QString::fromUtf8(f.readAll()));
            UiScale::apply(def::FONT_COMP);   // 字体视觉尺寸补偿基线
            return;
        }
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));   // 全局微软雅黑
    loadStyle(app);

    WinManager::instance()->start();   // 单例：统一管理窗口的创建/显示/跳转
    return app.exec();
}
