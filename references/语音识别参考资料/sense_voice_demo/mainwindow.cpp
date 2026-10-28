#include "mainwindow.h"

#include <QVBoxLayout>         // 垂直布局
#include <QHBoxLayout>         // 水平布局
#include <QGridLayout>         // 网格布局
#include <QGroupBox>           // 分组框（带标题的容器）
#include <QFileDialog>         // 文件/文件夹选择对话框
#include <QMessageBox>         // 消息提示框
#include <QFileInfo>           // 文件信息（虽然代码中未直接使用，但可能用于扩展）
#include <QRegularExpression>  // 正则表达式（字段提取）
#include <QApplication>        // 应用程序对象（processEvents）
#include <QDateTime>           // 日期时间（虽然代码中未直接使用）

/**
 * @brief 构造函数
 *
 * 初始化流程：
 * ┌─────────────────────────────────────────────────────────────┐
 * │ 1. buildUi()      → 构建界面                                │
 * │ 2. new AudioRecorder → 创建录音器（主线程）                   │
 * │ 3. new SenseVoiceEngine → 创建识别引擎                       │
 * │ 4. new QThread     → 创建工作线程                           │
 * │ 5. moveToThread()  → 将引擎移到工作线程                     │
 * │ 6. thread->start() → 启动线程事件循环                       │
 * │ 7. connect()       → 建立信号槽连接                         │
 * └─────────────────────────────────────────────────────────────┘
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();

    // ========== 步骤 1：创建音频录制器 ==========
    // AudioRecorder 在主线程运行，音频采集是轻量 IO 操作
    m_recorder = new AudioRecorder(this);

    // ========== 步骤 2：创建语音识别引擎 ==========
    // 注意：不指定父对象（或指定 nullptr），因为后续要 moveToThread
    // 如果指定了父对象，moveToThread 会失败或产生警告
    m_sense = new SenseVoiceEngine();

    // ========== 步骤 3：创建工作线程 ==========
    m_senseThread = new QThread(this);

    // ========== 步骤 4：将引擎移到工作线程 ==========
    // 此后 m_sense 的所有槽函数（包括 recognizeAudio）都在 m_senseThread 中执行
    // 主线程保持 UI 响应，不被识别计算阻塞
    m_sense->moveToThread(m_senseThread);

    // ========== 步骤 5：启动线程事件循环 ==========
    m_senseThread->start();

    // ========== 步骤 6：建立信号槽连接 ==========

    // 连接 1：识别完成 → 主线程更新 UI
    // 自动使用 Qt::AutoConnection，检测到跨线程后转为 QueuedConnection
    // 确保 onRecognizeFinished 在主线程执行（安全操作 UI）
    connect(m_sense, &SenseVoiceEngine::transcriptionFinished,
            this, &MainWindow::onRecognizeFinished);

    // 连接 2：识别错误 → 主线程显示错误信息（Lambda 槽函数）
    connect(m_sense, &SenseVoiceEngine::errorOccurred, this,
            [this](const QString &e) {
                m_lblStatus->setText("错误: " + e);
            });

    // 连接 3：录音错误 → 主线程显示错误信息
    connect(m_recorder, &AudioRecorder::errorOccurred, this,
            [this](const QString &e) {
                m_lblStatus->setText("录音错误: " + e);
            });

    // 连接 4：音频就绪 → 工作线程执行识别（关键跨线程连接）
    // Qt::QueuedConnection 强制排队：信号在主线程发射，槽在工作线程执行
    // std::vector<float> 通过 Qt 元对象系统安全跨线程传递
    connect(this, &MainWindow::audioReady,
            m_sense, &SenseVoiceEngine::recognizeAudio,
            Qt::QueuedConnection);
}

/**
 * @brief 析构函数
 *
 * 线程安全退出流程：
 * 1. quit()  → 请求线程退出事件循环（优雅方式）
 * 2. wait()  → 阻塞等待线程真正结束（最多等待默认超时时间）
 * 3. delete  → 线程结束后安全删除引擎对象
 *
 * 注意：不要直接 delete m_sense 再 wait()，会导致线程访问已释放内存
 */
MainWindow::~MainWindow()
{
    m_senseThread->quit();   // 向工作线程发送退出请求
    m_senseThread->wait();   // 等待线程事件循环结束
    delete m_sense;          // 线程已结束，安全释放引擎
}

/**
 * @brief 构建用户界面
 *
 * 整体布局结构：
 * ┌────────────────────────────────────────────┐
 * │ QVBoxLayout (mainLay)                      │
 * │  ├── QGroupBox "模型设置"                    │
 * │  │   └── QHBoxLayout [标签+路径+按钮]       │
 * │  ├── QGroupBox "语音录入"                    │
 * │  │   └── QVBoxLayout                       │
 * │  │       ├── QHBoxLayout [录音+停止按钮]     │
 * │  │       └── QLabel 状态提示                 │
 * │  ├── QGroupBox "识别结果"                    │
 * │  │   └── QTextEdit (只读)                   │
 * │  ├── QGroupBox "建档信息"                    │
 * │  │   └── QGridLayout [3行表单+提交按钮]      │
 * │  └── QHBoxLayout [清空按钮]                 │
 * └────────────────────────────────────────────┘
 */
void MainWindow::buildUi()
{
    // ========== 窗口基础设置 ==========
    setWindowTitle("自助语音建档系统");
    resize(680, 560);  // 窗口初始尺寸：宽680，高560

    // ========== 中央部件与主布局 ==========
    QWidget *central = new QWidget(this);
    setCentralWidget(central);  // QMainWindow 必须设置中央部件
    QVBoxLayout *mainLay = new QVBoxLayout(central);
    mainLay->setSpacing(12);           // 子控件间距 12 像素
    mainLay->setContentsMargins(20, 16, 20, 16);  // 左、上、右、下边距

    // ==================== 区域 1：模型设置 ====================
    QGroupBox *grpModel = new QGroupBox("模型设置", this);
    QHBoxLayout *layModel = new QHBoxLayout(grpModel);

    m_lblModelPath = new QLabel("未加载", this);
    m_lblModelPath->setStyleSheet("color:#888;");  // 灰色文字表示未加载状态

    QPushButton *btnLoad = new QPushButton("选择模型文件夹", this);

    layModel->addWidget(new QLabel("模型:", this));
    layModel->addWidget(m_lblModelPath, 1);  // stretch=1，占据剩余空间
    layModel->addWidget(btnLoad);
    mainLay->addWidget(grpModel);

    // ==================== 区域 2：语音录入 ====================
    QGroupBox *grpRec = new QGroupBox("语音录入", this);
    QVBoxLayout *layRec = new QVBoxLayout(grpRec);
    QHBoxLayout *layBtn = new QHBoxLayout();

    // --- 开始录音按钮（蓝色主题）---
    m_btnRecord = new QPushButton("开始录音", this);
    m_btnRecord->setEnabled(false);  // 初始禁用，模型加载后才可用
    m_btnRecord->setMinimumHeight(44);  // 最小高度，便于触摸操作
    // 样式表：蓝色背景、白色文字、圆角、悬停变深、禁用变灰
    m_btnRecord->setStyleSheet(
        "QPushButton{background:#2563eb;color:#fff;border:none;"
        "border-radius:5px;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#1d4ed8;}"
        "QPushButton:disabled{background:#94a3b8;}");

    // --- 停止录音按钮（红色主题）---
    m_btnStop = new QPushButton("停止录音", this);
    m_btnStop->setEnabled(false);  // 初始禁用，开始录音后才可用
    m_btnStop->setMinimumHeight(44);
    m_btnStop->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;"
        "border-radius:5px;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:disabled{background:#94a3b8;}");

    layBtn->addWidget(m_btnRecord);
    layBtn->addWidget(m_btnStop);
    layRec->addLayout(layBtn);

    // --- 状态提示标签 ---
    m_lblStatus = new QLabel("请先加载模型", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);  // 文字居中
    m_lblStatus->setStyleSheet("color:#6b7280;font-size:12px;");
    layRec->addWidget(m_lblStatus);
    mainLay->addWidget(grpRec);

    // ==================== 区域 3：识别结果 ====================
    QGroupBox *grpResult = new QGroupBox("识别结果", this);
    QVBoxLayout *layResult = new QVBoxLayout(grpResult);

    m_textResult = new QTextEdit(this);
    m_textResult->setReadOnly(true);  // 只读，用户不可编辑
    m_textResult->setPlaceholderText("识别文本...");  // 空时提示文字
    m_textResult->setMaximumHeight(100);  // 限制高度，节省空间
    layResult->addWidget(m_textResult);
    mainLay->addWidget(grpResult);

    // ==================== 区域 4：建档表单 ====================
    QGroupBox *grpForm = new QGroupBox("建档信息", this);
    QGridLayout *layForm = new QGridLayout(grpForm);
    layForm->setSpacing(8);  // 网格单元间距

    // --- 第 0 行：过敏史 ---
    layForm->addWidget(new QLabel("过敏史:", this), 0, 0);
    m_editAllergy = new QLineEdit(this);
    m_editAllergy->setPlaceholderText("从语音中提取...");
    m_editAllergy->setMinimumHeight(30);
    layForm->addWidget(m_editAllergy, 0, 1);

    // --- 第 1 行：体温 ---
    layForm->addWidget(new QLabel("体温:", this), 1, 0);
    m_editTemp = new QLineEdit(this);
    m_editTemp->setPlaceholderText("从语音中提取...");
    m_editTemp->setMinimumHeight(30);
    layForm->addWidget(m_editTemp, 1, 1);

    // --- 第 2 行：症状 ---
    layForm->addWidget(new QLabel("症状:", this), 2, 0);
    m_editSymptom = new QTextEdit(this);
    m_editSymptom->setPlaceholderText("从语音中提取...");
    m_editSymptom->setMaximumHeight(80);
    layForm->addWidget(m_editSymptom, 2, 1);

    // --- 第 3 行：提交按钮（跨 2 列）---
    QPushButton *btnSubmit = new QPushButton("确认提交", this);
    btnSubmit->setMinimumHeight(36);
    btnSubmit->setStyleSheet(
        "QPushButton{background:#16a34a;color:#fff;border:none;"
        "border-radius:5px;font-size:13px;font-weight:bold;}"
        "QPushButton:hover{background:#15803d;}");
    layForm->addWidget(btnSubmit, 3, 0, 1, 2);  // 行3，列0，占1行2列
    mainLay->addWidget(grpForm);

    // ==================== 区域 5：底部操作 ====================
    QHBoxLayout *layBottom = new QHBoxLayout();
    QPushButton *btnClear = new QPushButton("清空重录", this);
    layBottom->addStretch();  // 弹性空间将按钮推到右侧
    layBottom->addWidget(btnClear);
    mainLay->addLayout(layBottom);

    // ==================== 信号槽绑定 ====================
    connect(btnLoad,    &QPushButton::clicked, this, &MainWindow::onLoadModel);
    connect(m_btnRecord,&QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(m_btnStop,  &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(btnClear,   &QPushButton::clicked, this, &MainWindow::onClear);
    connect(btnSubmit,  &QPushButton::clicked, this, &MainWindow::onSubmit);
}

/**
 * @brief 加载模型按钮响应
 *
 * 流程：
 * 1. 弹出文件夹选择对话框
 * 2. 显示"正在加载"状态
 * 3. 调用 SenseVoiceEngine::loadModel()
 * 4. 根据结果更新 UI（成功=绿色，失败=红色）
 */
void MainWindow::onLoadModel()
{

    QString defaultModelPath = QString(PROJECT_SOURCE_DIR) + "/model";
    qDebug() << "defaultModelPath:" << defaultModelPath;
    // 校验 model 文件夹是否存在，不存在则用家目录
    if (!QDir(defaultModelPath).exists())
    {
        defaultModelPath = QDir::homePath();
    }
    // 选择文件夹（而非文件），因为模型包含多个文件（.onnx + tokens.txt）
    QString folderPath = QFileDialog::getExistingDirectory(
        this, "选择SenseVoice模型文件夹", defaultModelPath);
    if (folderPath.isEmpty()) return;  // 用户取消选择

    // 立即更新状态，让用户知道正在加载
    // QApplication::processEvents() 强制处理 pending 事件，确保标签立即刷新
    m_lblStatus->setText("正在加载模型...");
    QApplication::processEvents();

    // 调用引擎加载模型（同步阻塞调用，但模型加载通常很快）
    if (m_sense->loadModel(folderPath)) {
        // ========== 加载成功 ==========
        QDir dir(folderPath);
        m_lblModelPath->setText(dir.dirName());  // 显示文件夹名称
        m_lblModelPath->setStyleSheet("color:#16a34a;font-weight:bold;");  // 绿色加粗
        m_btnRecord->setEnabled(true);  // 启用录音按钮
        m_lblStatus->setText("模型加载成功");
    } else {
        // ========== 加载失败 ==========
        m_lblModelPath->setText("加载失败");
        m_lblModelPath->setStyleSheet("color:#dc2626;");  // 红色
    }
}

/**
 * @brief 开始录音按钮响应
 *
 * 前置检查：模型必须已加载（双重保险，按钮状态已控制）。
 * 录音成功后禁用"开始"按钮，启用"停止"按钮。
 */
void MainWindow::onStartRecording()
{
    // 防御性检查：虽然按钮已控制，但直接调用时仍需校验
    if (!m_sense->isModelLoaded()) {
        QMessageBox::warning(this, "提示", "请先加载模型");
        return;
    }

    // 启动录音，AudioRecorder 内部处理设备初始化和状态管理
    if (m_recorder->startRecording()) {
        m_btnRecord->setEnabled(false);  // 防止重复点击
        m_btnStop->setEnabled(true);     // 允许停止
        m_lblStatus->setText("正在录音...");
    }
}

/**
 * @brief 停止录音按钮响应
 *
 * 流程：
 * 1. 停止录音器
 * 2. 更新 UI 为"正在识别"
 * 3. 获取 Float32 音频数据
 * 4. 通过 audioReady 信号发送到工作线程识别
 *
 * 关键：std::move(data) 避免音频数据的深拷贝，提升性能
 */
void MainWindow::onStopRecording()
{
    // 停止音频采集
    m_recorder->stopRecording();

    // 禁用停止按钮，防止重复点击
    m_btnStop->setEnabled(false);

    // 更新状态，processEvents 确保 UI 立即刷新
    m_lblStatus->setText("正在识别...");
    QApplication::processEvents();

    // 获取标准化音频数据（16kHz Float32）
    auto data = m_recorder->getFloat32AudioData();

    // 防御：录音数据为空（如录音时间过短）
    if (data.empty()) {
        m_lblStatus->setText("录音数据为空");
        m_btnRecord->setEnabled(true);  // 允许重新录音
        return;
    }

    // 发射信号，std::move 转移所有权，避免 vector 拷贝
    // 信号通过 QueuedConnection 进入工作线程队列
    emit audioReady(std::move(data));
}

/**
 * @brief 识别完成回调（在主线程执行）
 * @param text 识别文本
 * @param elapsedMs 识别耗时
 *
 * 更新识别结果，恢复录音按钮，触发字段自动提取。
 */
void MainWindow::onRecognizeFinished(const QString &text, qint64 elapsedMs)
{
    // 显示识别文本
    m_textResult->setPlainText(text);

    // 显示耗时，保留 1 位小数（如"识别完成 (1.2 秒)"）
    m_lblStatus->setText(QString("识别完成 (%1 秒)").arg(elapsedMs / 1000.0, 0, 'f', 1));

    // 恢复录音按钮，允许下一条录音
    m_btnRecord->setEnabled(true);

    qDebug() << "原始识别文本：" << text;
    // 从识别文本提取医疗字段
    extractFields(text);
}

/**
 * @brief 从识别文本中提取结构化医疗信息
 * @param text 语音识别原始文本
 *
 * 提取规则（按优先级）：
 *
 * 1. 过敏史：
 *    - 模式 A："(过敏史?是为?|对)(\\S+?)(过敏|有敏感反应)"
 *      例："我对青霉素过敏" → 提取"青霉素"
 *    - 模式 B：文本包含"过敏"或"敏感"但未匹配模式 A
 *      回退：将整句填入，供人工修正
 *
 * 2. 体温：
 *    - 模式："(\\d{2}\\.?\\d?)\\s*[度℃]"
 *      例："体温三十七度五" → 提取"37.5℃"
 *      匹配 2 位数字 + 可选小数位 + 度/℃
 *
 * 3. 症状：
 *    - 模式 A："(症状?是为?|我|感觉|感到)(\\S+.*)"
 *      例："我感觉头疼恶心" → 提取"头疼恶心"
 *    - 模式 B：未匹配时，将整句作为症状描述
 */


void MainWindow::extractFields(const QString &rawText)
{
    if (rawText.isEmpty()) return;

    // 1. 预处理：只替换逗号和分号，保留句号（避免和小数点混淆）
    QString text = rawText;
    text.replace("，", ",").replace("；", ";");
    // 注意：不替换 "。" 为 "."

    // 2. 分割：只用逗号和分号分割
    QStringList segments = text.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);

    QString foundAllergy = "";
    QString foundTemp = "";
    QString foundSymptom = "";

    for (const QString &seg : segments) {
        QString s = seg.trimmed();
        if (s.isEmpty()) continue;

        // --- A. 提取体温 ---
        if (s.contains(QRegularExpression("温|度|℃"))) {
            QRegularExpression tempRe(".*?(\\d+(?:\\.\\d+)?)\\s*(?:度|℃)");
            QRegularExpressionMatch m = tempRe.match(s);

            if (m.hasMatch()) {
                QString val = m.captured(1);
                double tempVal = val.toDouble();
                if (tempVal > 30 && tempVal < 45) {
                    foundTemp = val + "℃";
                }
            }
        }

        // --- B. 提取过敏史 ---
        if (s.contains("过敏") || s.contains("敏感")) {
            if (s.contains(QRegularExpression("没?有|无"))) {
                foundAllergy = "无";
            } else {
                QRegularExpression duiRe("对(.+?)(?:过敏|敏感)");
                QRegularExpressionMatch mDui = duiRe.match(s);
                if (mDui.hasMatch()) {
                    foundAllergy = mDui.captured(1);
                } else {
                    QStringList parts = s.split(QRegularExpression("过敏|敏感"));
                    if (!parts.isEmpty()) {
                        QString candidate = parts[0];
                        candidate.remove(QRegularExpression("^.*(症状|是|为)[:：]?"));
                        if (!candidate.trimmed().isEmpty()) {
                            foundAllergy = candidate.trimmed();
                        }
                    }
                }
            }
        }

        // --- C. 提取症状 ---
        else {
            if (!s.contains(QRegularExpression("^\\d+.*度$"))) {
                QString symptomText = s;
                symptomText.remove(QRegularExpression("^.*(症状|感觉|感到|目前|主要)[:：是]?"));
                if (!symptomText.trimmed().isEmpty() && !symptomText.contains("体温")) {
                    foundSymptom += (foundSymptom.isEmpty() ? "" : "，") + symptomText.trimmed();
                }
            }
        }
    }

    // 3. 回退逻辑
    if (foundSymptom.isEmpty()) {
        QString remaining = text;
        remaining.remove(QRegularExpression("\\d+\\.?\\d*\\s*[度℃]"));
        remaining.remove(QRegularExpression(".*(过敏|敏感).*?[,.]"));
        remaining.remove(QRegularExpression("^.*(症状|是|为)[:：]?"));
        if (!remaining.trimmed().isEmpty()) {
            foundSymptom = remaining.trimmed();
        }
    }

    // 4. 更新 UI
    if (!foundAllergy.isEmpty()) m_editAllergy->setText(foundAllergy);
    if (!foundTemp.isEmpty())    m_editTemp->setText(foundTemp);
    if (!foundSymptom.isEmpty()) m_editSymptom->setPlainText(foundSymptom);
}

/**
 * @brief 清空所有输入和结果
 * 重置界面状态，便于重新录音建档。
 */
void MainWindow::onClear()
{
    m_textResult->clear();
    m_editAllergy->clear();
    m_editTemp->clear();
    m_editSymptom->clear();
    m_lblStatus->setText("已清空");
}

/**
 * @brief 提交建档信息
 *
 * 校验规则：至少录入体温或症状（避免空提交）。
 * 弹出信息框展示完整建档内容，供用户确认。
 */
void MainWindow::onSubmit()
{
    // 获取各字段内容并去除首尾空白
    QString allergy = m_editAllergy->text().trimmed();
    QString temp    = m_editTemp->text().trimmed();
    QString symptom = m_editSymptom->toPlainText().trimmed();

    // 校验：体温或症状至少填一项
    if (symptom.isEmpty() && temp.isEmpty()) {
        QMessageBox::warning(this, "提示", "请至少录入体温或症状");
        return;
    }

    // 格式化展示信息，空字段显示默认值
    QString info = QString("过敏史: %1\n体温: %2\n症状: %3")
                       .arg(allergy.isEmpty() ? "无" : allergy,
                            temp.isEmpty() ? "未测" : temp,
                            symptom.isEmpty() ? "无" : symptom);

    // 弹出确认对话框
    QMessageBox::information(this, "提交成功", "建档信息:\n\n" + info);

    // 调试输出到控制台
    qDebug() << "Submit:" << info;
}