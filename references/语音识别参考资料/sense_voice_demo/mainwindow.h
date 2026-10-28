#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>       // Qt 主窗口基类
#include <QThread>           // Qt 线程类，用于将识别引擎放到独立线程
#include <QLineEdit>         // 单行文本输入框（过敏史、体温）
#include <QTextEdit>         // 多行文本编辑框（识别结果、症状）
#include <QLabel>            // 文本标签（状态提示、模型路径）
#include <QPushButton>       // 按钮控件（录音、停止、加载等）
#include "sensevoiceengine.h" // SenseVoice 语音识别引擎
#include "audiorecorder.h"    // 音频录制器

/**
 * @class MainWindow
 * @brief 自助语音建档系统主窗口
 *
 * 应用功能：
 * - 加载 SenseVoice 语音识别模型
 * - 录制用户语音并实时识别
 * - 从识别文本中自动提取医疗信息（过敏史、体温、症状）
 * - 表单展示与手动修正
 * - 提交建档信息
 *
 * 线程架构：
 * - 主线程：UI 交互、音频录制（AudioRecorder 在主线程）
 * - 工作线程：语音识别（SenseVoiceEngine 在 m_senseThread 中）
 *   避免识别计算阻塞 UI，保证界面流畅响应
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT  // Qt 元对象系统宏，启用信号槽、运行时类型信息

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，通常为 nullptr（顶级窗口）
     *
     * 构造时完成：
     * 1. 构建 UI 界面
     * 2. 创建 AudioRecorder（主线程）
     * 3. 创建 SenseVoiceEngine 并移入独立线程
     * 4. 建立跨线程信号槽连接
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 清理顺序：
     * 1. 退出工作线程事件循环（quit）
     * 2. 等待线程真正结束（wait），防止资源泄漏
     * 3. 删除 SenseVoiceEngine（线程已结束，安全删除）
     *
     * 注意：AudioRecorder 有父对象 this，由 Qt 对象树自动释放
     */
    ~MainWindow() override;

private:
    /**
     * @brief 构建完整 UI 界面
     *
     * 界面分区（从上到下）：
     * ┌─────────────────────────────┐
     * │ 模型设置区（文件夹选择）      │
     * ├─────────────────────────────┤
     * │ 语音录入区（录音按钮+状态）   │
     * ├─────────────────────────────┤
     * │ 识别结果区（只读文本框）      │
     * ├─────────────────────────────┤
     * │ 建档表单区（过敏史/体温/症状）│
     * ├─────────────────────────────┤
     * │ 底部操作区（清空重录）        │
     * └─────────────────────────────┘
     */
    void buildUi();

private slots:
    /**
     * @brief 加载模型按钮槽函数
     *
     * 弹出文件夹选择对话框，加载 SenseVoice 模型文件。
     * 成功后将"开始录音"按钮置为可用状态。
     */
    void onLoadModel();

    /**
     * @brief 开始录音按钮槽函数
     *
     * 前置检查：模型必须已加载。
     * 启动 AudioRecorder 录音，更新 UI 状态。
     */
    void onStartRecording();

    /**
     * @brief 停止录音按钮槽函数
     *
     * 停止录音，获取 Float32 音频数据，
     * 通过 audioReady 信号发送到工作线程进行识别。
     */
    void onStopRecording();

    /**
     * @brief 识别完成槽函数
     * @param text 识别出的文本
     * @param elapsedMs 识别耗时（毫秒）
     *
     * 接收来自 SenseVoiceEngine 工作线程的识别结果，
     * 更新结果文本框，并触发字段提取。
     */
    void onRecognizeFinished(const QString &text, qint64 elapsedMs);

    /**
     * @brief 从识别文本中提取医疗字段
     * @param text 识别出的原始文本
     *
     * 使用正则表达式提取：
     * - 过敏史：匹配"对XX过敏"等模式
     * - 体温：匹配"XX度/℃"等模式
     * - 症状：匹配"症状是XX"等模式
     */
    void extractFields(const QString &text);

    /**
     * @brief 清空所有输入和结果
     * 重置界面到初始状态，便于重新录音建档。
     */
    void onClear();

    /**
     * @brief 提交建档信息
     *
     * 校验：至少录入体温或症状。
     * 弹出确认对话框展示完整建档信息。
     */
    void onSubmit();

signals:
    /**
     * @brief 音频数据就绪信号
     * @param data Float32 音频数据向量（通过 std::move 高效传递）
     *
     * 跨线程传输：主线程（录音停止后）→ 工作线程（SenseVoiceEngine）
     * 使用 Qt::QueuedConnection 确保线程安全
     */
    void audioReady(std::vector<float> data);

private:
    // ==================== 核心引擎 ====================
    SenseVoiceEngine *m_sense;       // 语音识别引擎（SenseVoice）
    QThread *m_senseThread;          // 识别引擎工作线程
    AudioRecorder *m_recorder;       // 音频录制器（在主线程运行）

    // ==================== UI 控件 ====================
    QLabel *m_lblModelPath;          // 显示当前模型文件夹名称
    QLabel *m_lblStatus;             // 状态提示标签（模型加载/录音/识别等状态）
    QPushButton *m_btnRecord;        // "开始录音"按钮（蓝色）
    QPushButton *m_btnStop;          // "停止录音"按钮（红色）
    QTextEdit *m_textResult;         // 识别结果展示（只读）
    QLineEdit *m_editAllergy;        // 过敏史输入框（自动填充，可手动修改）
    QLineEdit *m_editTemp;           // 体温输入框（自动填充，可手动修改）
    QTextEdit *m_editSymptom;        // 症状输入框（自动填充，可手动修改）
};

#endif // MAINWINDOW_H