#ifndef HOMEVIEW_H
#define HOMEVIEW_H
// 首页：左信息面板 + 右四宫格 + 待办（自包含演示数据与取消流程，对应 MD §4）
#include <QWidget>
#include <QVector>
#include <QString>
#include "util/PatientTypes.h"
#include "util/UIFactory.h"

class InfoPanel;
class QLabel;
class QVBoxLayout;

class HomeView : public QWidget
{
    Q_OBJECT
public:
    explicit HomeView(QWidget* parent = nullptr);

    void setProfile(const QString& gm, const QString& jw);
    void setJdDone(bool done);
    void setPanelWidth(int w);      // 窗口放大时左侧面板同步加宽
    // 检测页挂号成功后，WinManager 调此接口同步到待办（autoBook=重症自动预约）
    void addAppointment(const pd::Appointment& appt, bool autoBook);

signals:
    void cellClicked(pd::Page page);
    void logoutRequested();

private slots:
    void askCancel(int i);

private:
    QWidget* makeEntry(const QString& title, const QString& desc, pd::Page page);
    void refreshTodos();

    InfoPanel* m_panel = nullptr;
    QLabel* m_cellTitle = nullptr;
    QLabel* m_cellDesc  = nullptr;
    QVBoxLayout* m_todoBox = nullptr;
    QVector<pd::Appointment> m_todos;
};

#endif
