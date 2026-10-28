#ifndef TONGUE_DEF_H
#define TONGUE_DEF_H

// ============================================================================
// 舌苔业务定义：类别映射、判定规则、检测结果结构
// 类别顺序必须与训练 data.yaml 一致：
//   0:灰黑苔  1:镜面舌  2:薄白苔  3:白腻苔  4:黄腻苔
// ============================================================================

enum TongueClass
{
    TONGUE_HUI_HEI   = 0, // 灰黑苔
    TONGUE_JING_MIAN = 1, // 镜面舌
    TONGUE_BO_BAI    = 2, // 薄白苔
    TONGUE_BAI_NI    = 3, // 白腻苔
    TONGUE_HUANG_NI  = 4  // 黄腻苔
};

static const char *TONGUE_NAMES[5] = {
    "灰黑苔", "镜面舌", "薄白苔", "白腻苔", "黄腻苔"};

// 有效置信度阈值（百分制，项目手册要求 >=80% 判定为有效结果）
static const float TONGUE_CONF_THRESHOLD = 80.0f;

// 健康状态：0-健康 1-病体（薄白苔=健康，其余=病体）
inline int tongue_health(int cls)
{
    return (cls == TONGUE_BO_BAI) ? 0 : 1;
}

// 重症标记：灰黑苔/镜面舌=1（触发自动预约），其余=0
inline int tongue_severe(int cls)
{
    return (cls == TONGUE_HUI_HEI || cls == TONGUE_JING_MIAN) ? 1 : 0;
}

// 单次舌苔检测结论（阶段三串口协议直接复用此结构）
typedef struct
{
    int   cls_id;     // 类别id 0~4；-1=检测失败
    float confidence; // 置信度 0~100
    int   health;     // 0-健康 1-病体；失败时 -1
    int   severe;     // 0-否 1-是；失败时 -1
    int   valid;      // 1-有效 0-无效
} TongueResult;

#endif // TONGUE_DEF_H
