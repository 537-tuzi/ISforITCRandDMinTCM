#ifndef INFOPANEL_H
#define INFOPANEL_H
// 左侧个人信息面板（首页/检测/记录三页复用）
// 2026-10-30 决定：只保留 姓名/年龄/医保账号/手机号 四项，纯静态展示，无同步接口
#include <QWidget>

class QPushButton;

class InfoPanel : public QWidget
{
    Q_OBJECT
public:
    explicit InfoPanel(QWidget* parent = nullptr);

signals:
    void logoutClicked();
};

#endif // INFOPANEL_H
