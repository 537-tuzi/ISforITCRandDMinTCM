#ifndef UISCALE_H
#define UISCALE_H
// 界面缩放：MainWindow 按窗口宽度 / 设计宽度 800 得出因子；
// apply() 将 style.qss 原稿中所有 px 值等比放大后重新应用到全局，
// 使窗口最大化时容器与内容同比例放大。代码侧固定尺寸（弹窗宽等）用 current() 换算。
#include <QString>
#include <QtGlobal>

class QApplication;

namespace UiScale {
void init(QApplication* app, const QString& styleTemplate);  // main.cpp 注入样式原稿
void apply(qreal scale);                                     // 重建并应用缩放样式表
qreal current();                                             // 当前因子，默认 1.0
} // namespace UiScale
#endif
