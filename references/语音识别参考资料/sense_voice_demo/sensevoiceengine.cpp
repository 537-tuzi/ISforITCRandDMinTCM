#include "sensevoiceengine.h"

#include <cstring>           // C 标准字符串操作：memset 等
#include <QDebug>            // Qt 调试输出
#include <QFile>             // 文件存在性检查
#include <QElapsedTimer>     // 高精度计时器，测量识别耗时

// ============================================================
// extern "C" 块：包裹 C 语言头文件
//
// 原因：
// - Sherpa-ONNX 的 c-api.h 是纯 C 头文件
// - C++ 编译器会对函数名进行修饰（name mangling）
// - extern "C" 告诉编译器使用 C 链接约定，确保函数名正确解析
// ============================================================
extern "C" {
#include "c-api.h"
}

/**
 * @brief 构造函数
 *
 * 设计说明：
 * - 采用延迟初始化策略，构造时不加载模型
 * - 模型通常较大（几百 MB），应在需要时加载，避免启动耗时
 */
SenseVoiceEngine::SenseVoiceEngine(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 析构函数
 *
 * 资源释放：
 * - Sherpa-ONNX 的识别器必须通过 DestroyOfflineRecognizer 显式销毁
 * - 不销毁会导致：模型内存泄漏、ONNX Runtime 会话未关闭、文件句柄未释放
 * - 销毁后将指针置 nullptr，防止悬空指针
 */
SenseVoiceEngine::~SenseVoiceEngine()
{
    if(m_recognizer)
    {
        // 头文件原生函数：DestroyOfflineRecognizer（无前缀 SherpaOnnx_）
        // 释放识别器及其内部所有资源（模型、会话、内存池等）
        DestroyOfflineRecognizer(m_recognizer);
        m_recognizer = nullptr;
    }
    m_modelReady = false;
}

/**
 * @brief 加载 SenseVoice 模型
 *
 * 完整流程：
 * ┌─────────────────────────────────────────────────────────────┐
 * │ 1. 拼接模型文件路径                                          │
 * │ 2. 检查文件存在性（model.int8.onnx + tokens.txt）           │
 * │ 3. 初始化配置结构体（memset 清零）                            │
 * │ 4. 填充 SenseVoice 专用配置参数                               │
 * │ 5. 调用 CreateOfflineRecognizer 创建识别器                    │
 * │ 6. 检查返回值，更新状态标志                                   │
 * └─────────────────────────────────────────────────────────────┘
 */
bool SenseVoiceEngine::loadModel(const QString &modelFolderPath)
{
    // ========== 步骤 1：拼接完整文件路径 ==========
    // 模型文件：INT8 量化后的 ONNX 模型，体积约为 FP32 的 1/4
    QString modelPath = modelFolderPath + "/model.int8.onnx";
    // 词表文件：token 到文本的映射表，模型推理输出的是 token ID
    QString tokensPath = modelFolderPath + "/tokens.txt";

    // ========== 步骤 2：调试日志 - 打印路径和文件状态 ==========
    // 便于排查"文件找不到"等常见问题
    qDebug() << "选中模型目录：" << modelFolderPath;
    qDebug() << "模型文件路径：" << modelPath << " 是否存在：" << QFile::exists(modelPath);
    qDebug() << "词表文件路径：" << tokensPath << " 是否存在：" << QFile::exists(tokensPath);

    // ========== 步骤 3：文件存在性校验 ==========
    if(!QFile::exists(modelPath) || !QFile::exists(tokensPath))
    {
        // 任一文件缺失即报错，防止后续 CreateOfflineRecognizer 崩溃
        emit errorOccurred("模型文件夹缺少 model.int8.onnx / tokens.txt");
        m_modelReady = false;
        return false;
    }

    // ========== 步骤 4：初始化配置结构体 ==========
    // SherpaOnnxOfflineRecognizerConfig 是复杂的嵌套结构体
    // 必须用 memset 清零，避免未初始化字段包含垃圾值导致异常行为
    SherpaOnnxOfflineRecognizerConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    // ========== 步骤 5：填充 SenseVoice 专用配置 ==========
    //
    // 配置结构体层次（简化）：
    // cfg
    // ├── model_config
    // │   ├── sense_voice          ← SenseVoice 模型专属配置
    // │   │   ├── model            : ONNX 模型文件路径
    // │   │   ├── language         : 语言设置，"auto" 自动检测
    // │   │   └── use_itn          : 是否启用 ITN（Inverse Text Normalization）
    // │   ├── tokens               : 词表文件路径
    // │   ├── num_threads          : ONNX Runtime 推理线程数
    // │   └── provider             : 计算后端（cpu/cuda/coreml 等）
    // └── ...（其他通用配置，已 memset 清零）

    // ========== 修复：用局部std::string缓存，生命周期覆盖整个cfg使用阶段 ==========
    std::string strModelPath = modelPath.toStdString();
    std::string strTokensPath = tokensPath.toStdString();
    std::string strLanguage = "auto";
    std::string strProvider = "cpu";
    cfg.model_config.sense_voice.model = strModelPath.c_str();
    // language = "auto": SenseVoice 支持多语言，自动识别输入音频语言
    cfg.model_config.sense_voice.language = strLanguage.c_str();
    // use_itn = 1: 开启 ITN，将口语化数字转为阿拉伯数字，添加标点
    // 例如："我的电话是一三六" → "我的电话是136"
    cfg.model_config.sense_voice.use_itn = 1;

    cfg.model_config.tokens = strTokensPath.c_str();
    // num_threads = 4: 使用 4 个线程并行推理，平衡速度和 CPU 占用
    // 建议值：物理核心数 - 1，或根据设备性能调整
    cfg.model_config.num_threads = 4;
    // provider = "cpu": 使用 CPU 推理，无需 GPU
    // 可选值："cuda" (NVIDIA), "rocm" (AMD), "coreml" (Apple) 等
    cfg.model_config.provider = "cpu";

    // ========== 步骤 6：创建识别器 ==========
    // CreateOfflineRecognizer 内部完成：
    // - 加载 ONNX 模型到 ONNX Runtime
    // - 解析词表文件
    // - 初始化解码图（decoding graph）
    // - 分配推理所需的内存池
    m_recognizer = CreateOfflineRecognizer(&cfg);

    if(!m_recognizer)
    {
        // 创建失败可能原因：
        // - ONNX 模型文件损坏
        // - 词表格式错误
        // - 内存不足
        // - ONNX Runtime 后端不支持
        emit errorOccurred("SenseVoice 模型初始化失败");
        m_modelReady = false;
        return false;
    }

    m_modelReady = true;
    return true;
}

/**
 * @brief 查询模型加载状态
 */
bool SenseVoiceEngine::isModelLoaded() const
{
    return m_modelReady;
}

/**
 * @brief 执行离线语音识别
 * @param audioData 16kHz Float32 音频数据
 *
 * 识别流程（Sherpa-ONNX 离线识别范式）：
 * ┌─────────────────────────────────────────────────────────────┐
 * │ 1. 状态检查 → 确保模型已加载                                │
 * │ 2. 启动计时 → 测量识别耗时                                  │
 * │ 3. 创建流   → CreateOfflineStream（每个音频对应一个流）      │
 * │ 4. 送音频   → AcceptWaveformOffline（送入原始波形）          │
 * │ 5. 解码     → DecodeOfflineStream（执行神经网络推理+解码）   │
 * │ 6. 取结果   → GetOfflineStreamResult（获取文本）             │
 * │ 7. 释放资源 → DestroyOfflineRecognizerResult               │
 * │             → DestroyOfflineStream                          │
 * │ 8. 发射信号 → transcriptionFinished                         │
 * └─────────────────────────────────────────────────────────────┘
 *
 * 重要说明：
 * - 这是"离线识别"（Offline/Non-streaming），必须等音频全部送入后才能解码
 * - 适用于短音频（<30秒），长音频建议使用流式识别
 * - 当前实现为同步阻塞调用，若需异步应在独立线程中执行
 */
void SenseVoiceEngine::recognizeAudio(std::vector<float> audioData)
{
    // ========== 步骤 1：前置状态检查 ==========
    if(!m_modelReady || !m_recognizer)
    {
        emit errorOccurred("未加载模型");
        return;
    }

    // ========== 步骤 2：启动性能计时 ==========
    QElapsedTimer timer;
    timer.start();

    // ========== 步骤 3：创建离线识别流 ==========
    // OfflineStream 是单次识别的上下文，包含：
    // - 音频波形缓冲区
    // - 神经网络中间结果缓存
    // - 解码状态（如 CTC 路径、注意力权重等）
    SherpaOnnxOfflineStream* stream = CreateOfflineStream(m_recognizer);

    // ========== 步骤 4：送入音频波形数据 ==========
    // 参数说明：
    //   stream    : 离线流对象
    //   16000     : 采样率（Hz），必须与音频数据实际采样率一致
    //   audioData.data() : 音频数据指针（Float32 数组）
    //   (int32_t)audioData.size() : 采样点数量（总样本数）
    //
    // 注意：
    // - 采样率不匹配会导致识别结果错误（如语速变快/变慢）
    // - 数据范围应为 [-1.0, 1.0]，与 AudioRecorder::getFloat32AudioData() 输出一致
    AcceptWaveformOffline(stream, 16000, audioData.data(), (int32_t)audioData.size());

    // ========== 步骤 5：执行解码 ==========
    // DecodeOfflineStream 内部执行：
    // 1. 特征提取（如 80 维 log Mel-filterbank）
    // 2. Encoder 前向传播（SenseVoice 的声学模型）
    // 3. Decoder 解码（CTC/Attention/Transducer 等）
    // 4. 语言模型重打分（如配置了 LM）
    DecodeOfflineStream(m_recognizer, stream);

    // ========== 步骤 6：获取识别结果 ==========
    // 返回的 result 是 const 指针， Sherpa-ONNX 内部管理其内存
    // result->text 是 UTF-8 编码的 C 字符串
    const SherpaOnnxOfflineRecognizerResult* res = GetOfflineStreamResult(stream);

    // 将 UTF-8 C 字符串转换为 Qt QString，支持中文等多语言
    QString resultText = QString::fromUtf8(res->text);

    // 获取识别耗时
    qint64 costMs = timer.elapsed();

    // ========== 步骤 7：释放资源（重要！防止内存泄漏） ==========
    //
    // 释放顺序：先结果，后流
    //
    // DestroyOfflineRecognizerResult:
    // - 释放 result 指向的内存（文本、时间戳、tokens 等）
    // - 不调用会导致每次识别后内存持续增长
    DestroyOfflineRecognizerResult(res);

    // DestroyOfflineStream:
    // - 释放 stream 对象及其内部所有缓冲区
    // - 每个 stream 独立分配内存，必须显式销毁
    DestroyOfflineStream(stream);

    // ========== 步骤 8：通知识别完成 ==========
    emit transcriptionFinished(resultText, costMs);
}