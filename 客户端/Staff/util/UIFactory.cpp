#include "UIFactory.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>

namespace uif {

ClickFrame::ClickFrame(QWidget* parent) : QFrame(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickFrame::mousePressEvent(QMouseEvent*)
{
    emit clicked();
}

QWidget* makeFieldRow(const QString& key, const QString& val,
                      const QString& rowObj, const QString& valObj)
{
    QWidget* w = new QWidget;
    if (!rowObj.isEmpty()) w->setObjectName(rowObj);
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 6, 0, 6);
    h->setSpacing(8);
    QLabel* b = new QLabel(key);
    b->setObjectName("liKey");
    b->setFixedWidth(88);
    QLabel* s = new QLabel(val);
    s->setObjectName(valObj.isEmpty() ? QStringLiteral("liVal") : valObj);
    s->setWordWrap(true);
    h->addWidget(b);
    h->addWidget(s, 1);
    return w;
}

QWidget* makeStackedRow(const QString& key, const QString& rowObj,
                        const QString& keyObj, const QString& valObj, QLabel** val)
{
    QWidget* w = new QWidget;
    if (!rowObj.isEmpty()) w->setObjectName(rowObj);
    QVBoxLayout* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 4, 0, 4);
    v->setSpacing(2);
    QLabel* b = new QLabel(key);
    b->setObjectName(keyObj);
    QLabel* value = new QLabel(QStringLiteral("—"));
    value->setObjectName(valObj);
    value->setWordWrap(true);
    v->addWidget(b);
    v->addWidget(value);
    if (val) *val = value;
    return w;
}

} // namespace uif
