#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QByteArray>
#include <vector>

// 前向声明 Qt 类，减少头文件依赖，加快编译速度
class QAudioSource;   // Qt6 音频输入源类（替代了 Qt5 的 QAudioInput）
class QBuffer;        // 内存缓冲区 IO 设备

/**
 * @class AudioRecorder
 * @brief 基于 Qt6 QAudioSource 的音频录制类
 *
 * 功能：
 * - 从默认音频输入设备录制音频
 * - 将录制数据存储在内存缓冲区中
 * - 提供 Int16 原始数据到 Float32 标准化数据的转换
 * - 支持录音状态查询和错误信号通知
 */
class AudioRecorder : public QObject
{
    Q_OBJECT  // Qt 元对象系统宏，支持信号槽机制

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针，用于 Qt 对象树内存管理
     */
    explicit AudioRecorder(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     * 自动停止录音并释放音频资源
     */
    ~AudioRecorder();

    /**
     * @brief 开始录音
     * @return 成功返回 true，失败（如设备不可用）返回 false
     *
     * 流程：
     * 1. 配置音频格式（16kHz, 单声道, Int16）
     * 2. 获取默认音频输入设备
     * 3. 创建 QAudioSource 并绑定到 QBuffer
     * 4. 开始录制，音频数据自动写入内存缓冲区
     */
    bool startRecording();

    /**
     * @brief 停止录音
     * 停止音频源，关闭缓冲区，计算录音时长并发射信号
     */
    void stopRecording();

    /**
     * @brief 查询当前是否正在录音
     * @return true 表示正在录音中
     */
    bool isRecording() const;

    /**
     * @brief 获取标准化的 Float32 音频数据
     * @return 范围 [-1.0, 1.0] 的浮点向量
     *
     * 转换说明：
     * - 原始数据为 16 位有符号整数（Int16），范围 [-32768, 32767]
     * - 除以 32768.0f 归一化到 [-1.0, 1.0]
     * - 适用于深度学习模型输入（如语音识别 ASR）
     */
    std::vector<float> getFloat32AudioData() const;

    /**
     * @brief 静态方法：检查系统是否有可用的音频输入设备
     * @return true 表示存在音频输入设备（麦克风等）
     */
    static bool hasAudioInput();

signals:
    /**
     * @brief 录音开始信号
     * 在 startRecording() 成功启动后发射
     */
    void recordingStarted();

    /**
     * @brief 录音停止信号
     * @param durationMs 录音持续时间（毫秒）
     * 在 stopRecording() 完成后发射
     */
    void recordingStopped(qint64 durationMs);

    /**
     * @brief 错误信号
     * @param errorMsg 错误描述字符串
     * 在设备未找到、格式不支持等情况下发射
     */
    void errorOccurred(const QString &errorMsg);

private:
    QAudioSource *m_audioSource = nullptr;  // 音频输入源对象，负责从麦克风采集数据
    QBuffer      *m_buffer      = nullptr;  // 内存缓冲区，作为 QAudioSource 的写入目标
    QByteArray    m_audioBytes;             // 存储原始音频字节数据的动态数组
    bool          m_isRecording = false;    // 录音状态标志，防止重复启动/停止
    qint64        m_startTime   = 0;        // 录音开始时间戳（毫秒），用于计算时长

    // 采样率常量：16kHz，语音识别（如 Whisper）的标准采样率
    static constexpr int SAMPLE_RATE = 16000;
};

#endif // AUDIORECORDER_H