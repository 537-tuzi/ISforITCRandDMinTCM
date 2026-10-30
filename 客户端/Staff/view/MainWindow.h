#ifndef STAFFMAINWINDOW_H
#define STAFFMAINWINDOW_H
// 工作人员主窗口：顶部导航条（按角色：医生4页/管理员5页）+ 内容区（手册 3.4/3.5）
#include <QWidget>
#include <QList>
#include "util/StaffTypes.h"

class QPushButton;
class QStackedWidget;
class ApptListView;
class DiagnoseView;
class HistoryView;
class MySchedView;
class DoctorView;
class DeptView;
class SchedMgrView;
class StatsView;
class LogView;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(int role, const QString& workNo, const QString& name,
                        QWidget* parent = nullptr);

    ApptListView*  apptList()  const { return m_appt;  }
    DiagnoseView*  diagnose()  const { return m_diag;  }
    HistoryView*   history()   const { return m_hist;  }
    MySchedView*   mySched()   const { return m_sched; }
    DoctorView*    doctors()   const { return m_docs;  }
    DeptView*      depts()     const { return m_depts; }
    SchedMgrView*  schedMgr()  const { return m_smgr;  }
    StatsView*     stats()     const { return m_stats; }
    LogView*       logs()      const { return m_logs;  }

    void setCurrent(sdt::Page page);

signals:
    void logoutRequested();

protected:
    void resizeEvent(QResizeEvent* ev) override;

private:
    QPushButton* makeNav(const QString& text, sdt::Page page);
    int m_role = 1;
    QString m_workNo, m_name;
    QList<QPushButton*> m_navs;
    QWidget* m_bar = nullptr;
    QStackedWidget* m_stack = nullptr;
    ApptListView*  m_appt  = nullptr;
    DiagnoseView*  m_diag  = nullptr;
    HistoryView*   m_hist  = nullptr;
    MySchedView*   m_sched = nullptr;
    DoctorView*    m_docs  = nullptr;
    DeptView*      m_depts = nullptr;
    SchedMgrView*  m_smgr  = nullptr;
    StatsView*     m_stats = nullptr;
    LogView*       m_logs  = nullptr;
};

#endif // STAFFMAINWINDOW_H
