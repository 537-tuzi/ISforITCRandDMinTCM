#ifndef PATIENT_TYPES_H
#define PATIENT_TYPES_H
// 界面层共享的数据结构（原型演示用，无持久化）
#include <QString>
#include <QStringList>
#include <QMap>

namespace pd {

struct Appointment {
    QString date;   // "10-30"
    QString week;   // "周五 · 今天"（待办第二行与取消弹窗显示）
    QString doc;    // "脾胃科 · 赵国安"
    QString slot;   // "晚班 19:00-21:00"
    bool booked = true;   // true=已预约 false=已取消
    bool soon   = false;  // 距就诊不足30分钟
};

// 主窗口页面标识（导航条与首页宫格共用，替代魔法串）
enum class Page { Home, Jd, Detect, Record };

enum class Sev { Ok, Warn, Red };

struct DetectResult {
    QString type, state, conf;
    Sev sev = Sev::Warn;
    bool autoBook = false;
    QStringList rep;
    QString tips;
};

struct VisitRecord {
    QString date, slot, doc, tongue, dia, rx, ad, conf, when;
};

// 排班：科室 -> 医生 -> 日期 -> 时段列表
using Sched = QMap<QString, QMap<QString, QMap<QString, QStringList> > >;

} // namespace pd
#endif
