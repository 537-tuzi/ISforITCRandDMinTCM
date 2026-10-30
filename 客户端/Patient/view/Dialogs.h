#ifndef DIALOGS_H
#define DIALOGS_H
// 四个弹窗（对应 MD §8）
#include <QDialog>
#include "util/PatientTypes.h"

class QComboBox;
class QLabel;

// 8.1 手动预约（四级联动）
class ApptDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ApptDialog(const pd::Sched& sched, QWidget* parent = nullptr);
    QStringList result() const;   // 科室/医生/日期/时段（未选全时含空串）
signals:
    void incomplete();
private:
    void fill(QComboBox* box, const QStringList& items, const QString& ph);
    QComboBox* m_dep = nullptr;
    QComboBox* m_doc = nullptr;
    QComboBox* m_day = nullptr;
    QComboBox* m_time = nullptr;
    pd::Sched m_sched;
};

// 8.2 自动预约（重症）
class AutoApptDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AutoApptDialog(const QString& sevType, QWidget* parent = nullptr);
};

// 8.3 取消预约确认
class CancelDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CancelDialog(const pd::Appointment& a, QWidget* parent = nullptr);
};

// 8.4 病例报告
class ReportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReportDialog(const pd::VisitRecord& r, QWidget* parent = nullptr);
};

#endif
