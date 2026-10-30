#include <QApplication>
#include <QFile>
#include <QFont>
#include "util/winmanager.h"
#include "util/UiScale.h"

static void loadStyle(QApplication& app)
{
    QStringList candidates;
    candidates << QStringLiteral(":/style.qss")
               << QCoreApplication::applicationDirPath() + "/style.qss"
               << QStringLiteral("style.qss");
    foreach (const QString& p, candidates) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            UiScale::init(&app, QString::fromUtf8(f.readAll()));
            return;
        }
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));
    loadStyle(app);

    WinManager::instance()->start();
    return app.exec();
}
