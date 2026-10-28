#ifndef SENSEVOICEENGINE_H
#define SENSEVOICEENGINE_H

#include <QObject>
#include <QString>
#include <vector>

// ============================================================
// 前向声明：Sherpa-ONNX C API 的核心结构体
//
// 说明：
// - Sherpa-ONNX 是开源的语音识别/语音合成工具包
// - 以下结构体定义在其 C API 头文件 c-api.h 中
// - 这里使用前向声明避免在头文件中暴露完整的 C API 依赖
// - 具体实现在 .cpp 文件中通过 extern "C" 引入
// ============================================================

// 离线识别器：管理模型加载、解码配置和识别资源
typedef struct SherpaOnnxOfflineRecognizer SherpaOnnxOfflineRecognizer;

// 离线音频流：承载单条音频数据的识别上下文
typedef struct SherpaOnnxOfflineStream SherpaOnnxOfflineStream;

// 识别结果：包含转录文本、时间戳、token 等识别输出
typedef struct SherpaOnnxOfflineRecognizerResult SherpaOnnxOfflineRecognizerResult;

/**
 * @class SenseVoiceEngine
 * @brief 基于 Sherpa-ONNX 的 SenseVoice 语音识别引擎封装
 *
 * 功能特性：
 * - 加载 SenseVoice 模型（ONNX 格式，支持 INT8 量化）
 * - 自动语言检测（language = "auto"）
 * - ITN（Inverse Text Normalization）数字/标点自动规整
 * - 异步识别结果通过 Qt 信号通知
 * - 支持 16kHz Float32 音频输入
 *
 * 模型文件要求（modelFolderPath 目录下）：
 *   - model.int8.onnx   : 量化后的 ONNX 模型文件
 *   - tokens.txt        : 词表文件，模型输出的 token 映射
 */
class SenseVoiceEngine : public QObject
{
    Q_OBJECT  // Qt 元对象系统宏，启用信号槽机制

public:
    /**
     * @brief 构造函数
     * @param parent 父对象，用于 Qt 对象树内存管理
     *
     * 注意：构造时不会加载模型，需显式调用 loadModel()
     */
    explicit SenseVoiceEngine(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     * 自动释放 Sherpa-ONNX 识别器资源
     */
    ~SenseVoiceEngine() override;

    /**
     * @brief 加载 SenseVoice 模型
     * @param modelFolderPath 模型文件夹路径（需包含 model.int8.onnx 和 tokens.txt）
     * @return true 表示加载成功，false 表示失败（文件缺失或模型初始化失败）
     *
     * 配置参数：
     * - language: "auto" 自动检测语言
     * - use_itn: 1 开启 ITN，自动将"一二三"转为"123"，添加标点
     * - num_threads: 4 使用 4 线程推理
     * - provider: "cpu" 使用 CPU 推理（也可设为 "cuda" 等）
     */
    bool loadModel(const QString &modelFolderPath);

    /**
     * @brief 查询模型是否已加载成功
     * @return true 表示模型就绪，可进行识别
     */
    bool isModelLoaded() const;

    /**
     * @brief 执行语音识别（离线/非流式）
     * @param audioData 16kHz 采样率的 Float32 音频数据，范围 [-1.0, 1.0]
     *
     * 识别流程：
     * 1. 创建离线流（CreateOfflineStream）
     * 2. 送入音频波形（AcceptWaveformOffline）
     * 3. 执行解码（DecodeOfflineStream）
     * 4. 获取结果文本并释放资源
     *
     * 注意：此方法在当前线程同步执行，若音频较长可能阻塞
     */
    void recognizeAudio(std::vector<float> audioData);

signals:
    /**
     * @brief 识别完成信号
     * @param text 识别出的文本结果
     * @param elapsedMs 识别耗时（毫秒），用于性能监控
     */
    void transcriptionFinished(const QString &text, qint64 elapsedMs);

    /**
     * @brief 错误信号
     * @param errMsg 错误描述
     * 在模型加载失败、文件缺失、未加载模型就识别等情况下发射
     */
    void errorOccurred(const QString &errMsg);

private:
    // Sherpa-ONNX 离线识别器实例，由 CreateOfflineRecognizer 创建
    SherpaOnnxOfflineRecognizer* m_recognizer = nullptr;

    // 模型加载状态标志，防止未加载模型时调用识别
    bool m_modelReady = false;
};

#endif // SENSEVOICEENGINE_H