#ifndef INFOPANEL_H
#define INFOPANEL_H
// 左侧个人信息面板（首页/检测/记录三页复用，对应 MD §4.1/§6.1/§7.1）
#include <QWidget>
#include <QString>

class QLabel;

class InfoPanel : public QWidget
{
    Q_OBJECT
public:
    // showPhone=显示手机号行；showStatus=显示建档状态行
    explicit InfoPanel(bool showPhone, bool showStatus, QWidget* parent = nullptr);

    void setProfile(const QString& gm, const QString& jw);
    void setJdDone(bool done);

signals:
    void logoutClicked();

private:
    QLabel* m_gm  = nullptr;
    QLabel* m_jw  = nullptr;
    QLabel* m_jd  = nullptr;
};

#endif
