#ifndef TOAST_H
#define TOAST_H
// 全局提示条：顶部居中 3.2s 自动消失（对应 MD §1.4）
#include <QString>
class QWidget;

class Toast
{
public:
    static void show(QWidget* window, const QString& msg, bool warn = false);
};

#endif
