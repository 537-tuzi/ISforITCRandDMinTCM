#ifndef MAINWINDOW_H
#define MAINWINDOW_H
// 主窗口：顶部导航条 + 内容区（对应 MD §2/§4）
#include <QWidget>
#include <QList>
#include "util/PatientTypes.h"

class QPushButton;
class QStackedWidget;
class HomeView;
class ProfileView;
class DetectView;
class RecordView;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    HomeView*    home()    const { return m_home; }
    ProfileView* profile() const { return m_profile; }
    DetectView*  detect()  const { return m_detect; }
    RecordView*  record()  const { return m_record; }

    void setCurrent(pd::Page page);
    void setJdTitle(const QString& title);   // 初次建档 -> 我的档案

signals:
    void navRequested(pd::Page page);

protected:
    void resizeEvent(QResizeEvent* ev) override;   // 窗口放大时同步放大内容

private:
    QPushButton* makeNav(const QString& text, pd::Page page);
    QList<QPushButton*> m_navs;
    QStackedWidget* m_stack = nullptr;
    QWidget* m_bar = nullptr;      // 顶部导航条（高度随缩放）
    QPushButton* m_navJd = nullptr;
    HomeView*    m_home = nullptr;
    ProfileView* m_profile = nullptr;
    DetectView*  m_detect = nullptr;
    RecordView*  m_record = nullptr;
};

#endif
