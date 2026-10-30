#ifndef STAFF_TYPES_H
#define STAFF_TYPES_H
// 医生端+管理员端（同一工程，手册 3.3"客户端工程2"）：界面层共享数据结构（原型演示用）
#include <QString>
#include <QStringList>
#include <QVector>

namespace sdt {

// 角色（手册 3.2.4：医生/管理员统一校验，角色分流）
enum Role { Doctor = 1, Admin = 2 };

// 页面标识（两个角色各自使用子集）
enum class Page {
    ApptList,    // 医生：当日预约患者列表
    Diagnose,    // 医生：问诊开方+病例生成
    History,     // 医生：历史诊疗病例
    MySchedule,  // 医生：个人排班周视图
    Doctors,     // 管理员：医师档案管理
    Depts,       // 管理员：科室管理
    SchedMgr,    // 管理员：排班调度
    Stats,       // 管理员：诊疗工作量统计
    Logs         // 管理员：系统日志管理
};

// 预约患者列表行（手册 3.4.2 表头：姓名/性别/年龄/身份证号(脱敏)/医保卡号/手机号/预约时段/状态）
struct PatientRow {
    QString name, gender, age;
    QString idCardMasked;   // 3301**********2215（服务端脱敏下发）
    QString cardNo, phone;  // 手机号：医生端问诊卡完整显示（3.4.4 图例），列表内脱敏
    QString slot, status;   // 上午 08:00-12:00 / 已预约
    bool severe = false;    // 重症标记（舌苔结果联动）
};

// 舌苔检测报告（医生端查看）
struct TongueReport {
    QString type, conf, state, when;   // 白腻苔 / 93% / 身体有恙 / 2026-10-20 09:12
    bool severe = false;
};

// 处方药材行（批量添加）
struct RxItem { QString name, dosage, usage; };

// 历史病例（含处方）
struct VisitCase {
    QString patient, date, dept, diagnosis, tongue;
    QVector<RxItem> rx;
};

// 排班格（医生查看/管理员调度共用）
struct SchedCell {
    QString doctor, dept, date;   // date: 2026-10-31
    int shift = 1;                // 1上午 2下午 3晚班
    int maxN = 20, booked = 0;
};

// 医师档案行（管理员）
struct DoctorRow {
    QString workNo, name, dept, title, phone;
    int status = 1;               // 0-已删除(逻辑删) 1-正常
};

// 科室行（管理员）
struct DeptRow { int id = 0; QString name, desc, leader; int status = 1; };

// 日志行（管理员）
struct LogRow {
    QString time, oper, type, desc, result, ip;
};

// 统计行（管理员：周/月工作量）
struct StatRow { QString doctor; int total = 0, done = 0; };

} // namespace sdt
#endif // STAFF_TYPES_H
