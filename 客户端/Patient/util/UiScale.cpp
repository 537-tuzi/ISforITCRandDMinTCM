#include "UiScale.h"
#include <QApplication>
#include <QRegularExpression>

namespace UiScale {
namespace {
QApplication* g_app = nullptr;
QString g_tpl;        // style.qss 原稿（未缩放）
qreal g_scale = 1.0;
}

void init(QApplication* app, const QString& styleTemplate)
{
    g_app = app;
    g_tpl = styleTemplate;
    apply(1.0);
}

/* 把原稿中所有 "NNpx" 等比取整替换后整体重设；scale<=1 时用原稿 */
void apply(qreal scale)
{
    if (!g_app) return;
    g_scale = scale;
    if (scale <= 1.001) {
        g_app->setStyleSheet(g_tpl);
        return;
    }
    static const QRegularExpression re(QStringLiteral("(\\d+(?:\\.\\d+)?)px"));
    QString scaled;
    scaled.reserve(g_tpl.size() + 256);
    qsizetype last = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(g_tpl);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        scaled += g_tpl.mid(last, m.capturedStart() - last);
        scaled += QString::number(qRound(m.captured(1).toDouble() * scale)) +
                  QStringLiteral("px");
        last = m.capturedEnd();
    }
    scaled += g_tpl.mid(last);
    g_app->setStyleSheet(scaled);
}

qreal current() { return g_scale; }

} // namespace UiScale
