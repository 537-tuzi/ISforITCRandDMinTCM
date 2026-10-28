#include "audiorecorder.h"

#include <QAudioSource>      // Qt6 音频输入核心类
#include <QAudioDevice>      // 音频设备信息
#include <QAudioFormat>      // 音频格式配置（采样率、声道、位深）
#include <QMediaDevices>     // 系统多媒体设备枚举（Qt6 新增）
#include <QBuffer>           // 内存 IO 设备，数据存储在 QByteArray 中
#include <QDateTime>         // 获取当前时间戳
#include <QDebug>            // Qt 调试输出

/**
 * @brief 构造函数
 * 仅初始化基类，音频资源在 startRecording() 中延迟创建
 */
AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 析构函数
 *
 * 安全清理策略：
 * 1. 如果正在录音，先调用 stopRecording() 停止，避免资源泄漏
 * 2. 删除音频源和缓冲区对象
 *
 * 注意：delete nullptr 在 C++ 中是安全的（无操作）
 */
AudioRecorder::~AudioRecorder()
{
    if (m_isRecording) stopRecording();
    delete m_audioSource;
    delete m_buffer;
}

/**
 * @brief 检查系统音频输入设备
 *
 * QMediaDevices::defaultAudioInput() 获取系统默认输入设备（如麦克风）。
 * 如果返回空设备（isNull()），表示没有可用的音频输入硬件。
 */
bool AudioRecorder::hasAudioInput()
{
    return !QMediaDevices::defaultAudioInput().isNull();
}

/**
 * @brief 启动录音流程
 *
 * 完整流程：
 * ┌─────────────────────────────────────────────────────────┐
 * │ 1. 检查状态 → 防止重复启动                               │
 * │ 2. 配置格式 → 16kHz / 单声道 / Int16                     │
 * │ 3. 获取设备 → 默认音频输入设备                            │
 * │ 4. 格式回退 → 如不支持则使用设备首选格式                   │
 * │ 5. 清理旧资源 → 防止内存泄漏                              │
 * │ 6. 创建对象 → QAudioSource + QBuffer                     │
 * │ 7. 开始录制 → 数据自动流入 m_audioBytes                   │
 * └─────────────────────────────────────────────────────────┘
 */
bool AudioRecorder::startRecording()
{
    // 状态检查：如果已经在录音中，拒绝重复启动
    if (m_isRecording) return false;

    // ========== 步骤 1：配置音频格式 ==========
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);           // 设置采样率为 16kHz
    format.setChannelCount(1);                   // 单声道（Mono），减少数据量
    format.setSampleFormat(QAudioFormat::Int16); // 16位有符号整数，CD 音质标准

    // ========== 步骤 2：获取默认音频输入设备 ==========
    QAudioDevice deviceInfo = QMediaDevices::defaultAudioInput();
    if (deviceInfo.isNull()) {
        // 设备未找到：可能是没有麦克风或驱动问题
        emit errorOccurred("未找到音频输入设备");
        return false;
    }

    // ========== 步骤 3：格式兼容性检查与回退 ==========
    // 并非所有设备都支持 16kHz/Int16，如果不支持则使用设备推荐格式
    if (!deviceInfo.isFormatSupported(format)) {
        format = deviceInfo.preferredFormat();
        qWarning() << "Using preferred format:"
                   << format.sampleRate() << "Hz";
        // 警告：如果回退到非 16kHz，后续 ASR 模型可能需要重采样
    }

    // ========== 步骤 4：清理历史资源 ==========
    // 删除旧的音频源和缓冲区，确保每次启动都是干净状态
    delete m_audioSource;
    delete m_buffer;
    m_audioBytes.clear();  // 清空之前录制的音频数据

    // ========== 步骤 5：创建音频源对象 ==========
    // QAudioSource 构造函数参数：设备信息、音频格式、父对象
    m_audioSource = new QAudioSource(deviceInfo, format, this);

    // 设置内部缓冲区大小：采样率 × 2字节(Int16) × 5秒
    // 即预留 5 秒的音频缓冲空间（约 160KB）
    m_audioSource->setBufferSize(SAMPLE_RATE * 2 * 5);

    // ========== 步骤 6：创建内存缓冲区 ==========
    // QBuffer 绑定到 m_audioBytes，所有写入操作实际存入 QByteArray
    // QIODevice::WriteOnly：只允许写入，模拟只写文件
    m_buffer = new QBuffer(&m_audioBytes, this);
    m_buffer->open(QIODevice::WriteOnly);

    // ========== 步骤 7：启动录音 ==========
    // start() 将 QAudioSource 与 QBuffer 连接，音频数据自动流入缓冲区
    m_audioSource->start(m_buffer);

    // 更新状态
    m_isRecording = true;
    m_startTime = QDateTime::currentMSecsSinceEpoch();  // 记录开始时间戳

    qDebug() << "Recording started.";
    emit recordingStarted();  // 通知 UI 或其他组件录音已开始
    return true;
}

/**
 * @brief 停止录音
 *
 * 流程：
 * 1. 停止音频源（停止从设备采集数据）
 * 2. 关闭缓冲区（结束写入模式）
 * 3. 计算录音时长
 * 4. 发射停止信号
 */
void AudioRecorder::stopRecording()
{
    // 状态保护：未在录音状态下调用直接返回
    if (!m_isRecording) return;

    // 停止音频采集，QAudioSource 不再向缓冲区写入数据
    m_audioSource->stop();

    // 关闭缓冲区，确保所有数据已刷新到 m_audioBytes
    m_buffer->close();

    // 重置录音状态
    m_isRecording = false;

    // 计算录音持续时间（当前时间 - 开始时间）
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - m_startTime;

    qDebug() << "Recording stopped." << duration << "ms,"
             << m_audioBytes.size() << "bytes";

    emit recordingStopped(duration);  // 通知调用方录音已结束及持续时间
}

/**
 * @brief 查询录音状态
 */
bool AudioRecorder::isRecording() const
{
    return m_isRecording;
}

/**
 * @brief 将原始 Int16 音频数据转换为 Float32 标准化数据
 *
 * 数据转换原理：
 * ┌────────────────────────────────────────────────────────┐
 * │ Int16 范围：[-32768, 32767]                            │
 * │ Float32 目标范围：[-1.0, 1.0]                          │
 * │ 转换公式：float = int16 / 32768.0f                     │
 * │                                                        │
 * │ 示例：                                                 │
 * │   Int16: 0      → Float32: 0.0                        │
 * │   Int16: 32767  → Float32: 0.9999695（接近 1.0）       │
 * │   Int16: -32768 → Float32: -1.0                       │
 * └────────────────────────────────────────────────────────┘
 *
 * 应用场景：
 * - 大多数深度学习音频模型（如 Whisper、Wav2Vec2）要求 Float32 输入
 * - 标准化后的数据均值为 0，方差适中，有利于模型收敛
 *
 * @return Float32 向量，每个元素范围 [-1.0, 1.0]
 */
std::vector<float> AudioRecorder::getFloat32AudioData() const
{
    // 计算采样点数量：总字节数 / 每个采样点的字节数（Int16 = 2 字节）
    const int count = m_audioBytes.size() / static_cast<int>(sizeof(int16_t));

    // 预分配 vector 容量，避免动态扩容开销
    std::vector<float> result(count);

    // 将 QByteArray 数据指针解释为 Int16 数组指针
    const auto *src = reinterpret_cast<const int16_t*>(m_audioBytes.constData());

    // 逐点转换：Int16 → Float32 并归一化
    for (int i = 0; i < count; ++i) {
        result[i] = static_cast<float>(src[i]) / 32768.0f;
    }

    return result;
}