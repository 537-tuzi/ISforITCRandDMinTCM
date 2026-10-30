#ifndef DETECTVIEW_H
#define DETECTVIEW_H
// 舌苔检测页：左信息面板 + 中摄像头 + 右报告（自包含五类结果库/演示循环/预约弹窗，对应 MD §6）
#include <QWidget>
#include <QList>
#include "util/PatientTypes.h"

class InfoPanel;
class QLabel;
class QPushButton;
class QFrame;
class QTimer;
class QEvent;
class QObject;

class DetectView : public QWidget
{
    Q_OBJECT
public:
    explicit DetectView(QWidget* parent = nullptr);

    void setProfile(const QString& gm, const QString& jw);   // 过敏史/既往病史同步到左面板
    void setJdDone(bool done);                               // 建档状态同步到左面板
    void setPanelWidth(int w);                               // 窗口放大时左面板同步加宽

signals:
    // 挂号成功 -> 首页待办同步（autoBook=true 为重症自动预约）
    void appointmentAdded(const pd::Appointment& appt, bool autoBook);
    void logoutClicked();

private slots:
    void onStart();
    void onReset();
    void onBook();          // 手动预约（四级联动弹窗）
    void tickScan();        // 扫描线动画

private:
    QWidget* makeStep(int n, const QString& text);
    QFrame* makeStat(const QString& title, QLabel** val);
    void clearResult();     // 结果区恢复默认态（再检测一次用）
    void placeCorners();    // 摄像头画面四角取景框摆位
    bool eventFilter(QObject* obj, QEvent* ev) override;

    InfoPanel* m_panel = nullptr;
    QLabel* m_type = nullptr;
    QLabel* m_state = nullptr;
    QLabel* m_conf  = nullptr;
    QLabel* m_repBody = nullptr;
    QLabel* m_repNote = nullptr;
    QFrame* m_repCard = nullptr;
    QLabel* m_tipsBody = nullptr;
    QFrame* m_tipsCard = nullptr;
    QPushButton* m_start = nullptr;
    QFrame* m_screen = nullptr;
    QFrame* m_scan = nullptr;
    QList<QFrame*> m_corners;   // 取景框四角（ctl/ctr/cbl/cbr）
    QTimer* m_scanTimer = nullptr;
    int m_scanY = 20;
    int m_detIdx = 1;           // 演示循环起始：白腻苔
};

#endif
