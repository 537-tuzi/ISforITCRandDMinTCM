#include "Toast.h"
#include "AppDefs.h"
#include <QLabel>
#include <QTimer>
#include <QStyle>
#include <QPointer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace {
// 全局单条提示：持续触发（条件不符反复点击）时每次 show 重置计时即常显；
// 停止触发 3.2s 后 500ms 淡出消失。不再多条追加叠放。
// g_toast 用 QPointer：挂到栈上弹窗（&dlg）时弹窗销毁会连带销毁 toast，
// QPointer 自动置空，下次 show 走重建分支，杜绝悬垂指针。
QPointer<QLabel> g_toast;
QTimer* g_hide = nullptr;          // 停止触发后开始淡出（g_toast 子对象，随之销毁）
QPropertyAnimation* g_fade = nullptr;
}

void Toast::show(QWidget* window, const QString& msg, bool warn)
{
    if (!window) return;
    if (!g_toast) {
        g_toast = new QLabel;
        g_toast->setAlignment(Qt::AlignCenter);
        g_toast->setWordWrap(true);
        QGraphicsOpacityEffect* fx = new QGraphicsOpacityEffect(g_toast);
        g_toast->setGraphicsEffect(fx);
        g_fade = new QPropertyAnimation(fx, "opacity", g_toast);
        g_fade->setDuration(def::TOAST_FADE_MS);
        g_fade->setStartValue(1.0);
        g_fade->setEndValue(0.0);
        QObject::connect(g_fade, &QPropertyAnimation::finished, g_toast, &QWidget::hide);
        g_hide = new QTimer(g_toast);
        g_hide->setSingleShot(true);
        QObject::connect(g_hide, &QTimer::timeout, g_toast, [] { g_fade->start(); });
    }

    g_toast->setParent(window);            // 跟随当前窗口（登录窗/主窗）
    g_toast->setObjectName(warn ? "toastWarn" : "toast");
    g_toast->style()->unpolish(g_toast);
    g_toast->style()->polish(g_toast);
    g_toast->setText(msg);
    g_toast->setMaximumWidth(window->width() * 76 / 100);   // 放宽换行上限，配合大字号
    g_toast->adjustSize();
    g_toast->move((window->width()  - g_toast->width())  / 2,
                  (window->height() - g_toast->height()) / 2);   // 屏幕正中

    g_fade->stop();                        // 重置状态：回到不透明
    static_cast<QGraphicsOpacityEffect*>(g_toast->graphicsEffect())->setOpacity(1.0);
    g_toast->show();
    g_toast->raise();
    g_hide->start(def::TOAST_VISIBLE_MS);  // 停止触发 3.2s 后开始淡出
}
