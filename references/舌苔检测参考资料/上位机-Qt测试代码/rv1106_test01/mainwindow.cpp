#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "SerialWorker.h"
#include "VideoReceiver.h"

#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTime>
#include <QDebug>          // 用于日志输出（可替换为状态栏）

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serialThread(nullptr)
    , m_serialWorker(nullptr)
    , m_netThread(nullptr)
    , m_videoReceiver(nullptr)
    , m_isVideoPlaying(false)
{
    ui->setupUi(this);

    // 刷新串口列表
    refreshPorts();

    // 【MODIFY】按钮固定文字：仅单帧推理
    ui->btn_action->setEnabled(false);
    ui->btn_action->setText("单帧拍照推理");

    // 状态标签初始显示
    ui->label_serial_status->setText("未连接");
    ui->label_net_status->setText("未连接");

    // 视频刷新定时器（30ms 刷新一次，约 33fps）
    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, &QTimer::timeout, this, &MainWindow::updateVideoDisplay);
    m_displayTimer->start(30);
}

MainWindow::~MainWindow()
{
    // 安全停止串口线程
    if (m_serialThread) {
        m_serialThread->quit();
        m_serialThread->wait();
        delete m_serialThread;
    }
    // 安全停止网络线程
    if (m_netThread) {
        m_netThread->quit();
        m_netThread->wait();
        delete m_netThread;
    }
    delete ui;
}

// ---------- 辅助函数 ----------
void MainWindow::refreshPorts()
{
    ui->combo_port->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        ui->combo_port->addItem("无可用串口");
        ui->combo_port->setEnabled(false);
    } else {
        for (const auto &info : ports) {
            // 显示格式：端口名 - 描述
            ui->combo_port->addItem(info.portName() + " - " + info.description());
        }
        ui->combo_port->setEnabled(true);
    }
    onLogMessage(tr("刷新串口，发现 %1 个设备").arg(ports.size()));
}

// ---------- UI 槽函数 ----------
void MainWindow::on_btn_serial_clicked()
{
    // ========== 已连接 → 断开流程（保留原invokeMethod，不加Q_ARG） ==========
    if (m_serialThread && m_serialThread->isRunning())
    {
        // 1. 异步下发stop指令给子线程，维持原有写法
        QMetaObject::invokeMethod(m_serialWorker, "stop", Qt::QueuedConnection);
        // 2. 请求线程事件循环退出
        m_serialThread->quit();

        // 3. 绑定线程finished信号，在线程真正结束后再清理UI，不阻塞主线程
        connect(m_serialThread, &QThread::finished, this, [this](){
            // 资源释放
            m_serialWorker->deleteLater();
            m_serialThread->deleteLater();
            m_serialWorker = nullptr;
            m_serialThread = nullptr;

            // 重置UI状态
            ui->combo_port->setEnabled(true);
            ui->combo_baud->setEnabled(true);
            ui->btn_serial->setText("连接串口");
            ui->btn_action->setEnabled(false);
            ui->label_serial_status->setText("未连接");
            ui->label_serial_status->setStyleSheet("color: black;");
            m_isVideoPlaying = false;
            onLogMessage("串口已断开");
        }, Qt::SingleShotConnection);
        return;
    }

    // ========== 连接流程不变 ==========
    QString portText = ui->combo_port->currentText();
    if (portText.startsWith("无可用串口"))
    {
        QMessageBox::warning(this, "警告", "没有可用的串口设备");
        return;
    }
    QString portName = portText.split(" ").first();
    qint32 baudRate = ui->combo_baud->currentText().toInt();

    m_serialThread = new QThread(this);
    m_serialWorker = new SerialWorker(portName, baudRate);
    m_serialWorker->moveToThread(m_serialThread);

    connect(m_serialThread, &QThread::started, m_serialWorker, &SerialWorker::start);
    // 线程结束自动释放worker
    connect(m_serialThread, &QThread::finished, m_serialWorker, &QObject::deleteLater);

    // 跨线程队列连接
    connect(m_serialWorker, &SerialWorker::connected, this, &MainWindow::onSerialConnected, Qt::QueuedConnection);
    connect(m_serialWorker, &SerialWorker::resultReceived, this, &MainWindow::onResultReceived, Qt::QueuedConnection);
    connect(m_serialWorker, &SerialWorker::logMessage, this, &MainWindow::onLogMessage, Qt::QueuedConnection);
    connect(m_serialWorker, &SerialWorker::inferenceResultReceived, this, &MainWindow::onInferenceResultReceived, Qt::QueuedConnection);

    m_serialThread->start();
}

void MainWindow::on_btn_net_clicked()
{
    // 已连接，断开（保留原invokeMethod写法）
    if (m_netThread && m_netThread->isRunning())
    {
        QMetaObject::invokeMethod(m_videoReceiver, "stop", Qt::QueuedConnection);
        m_netThread->quit();

        connect(m_netThread, &QThread::finished, this, [this](){
            m_videoReceiver->deleteLater();
            m_netThread->deleteLater();
            m_videoReceiver = nullptr;
            m_netThread = nullptr;

            ui->btn_net->setText("连接网络");
            ui->label_net_status->setText("未连接");
            ui->label_net_status->setStyleSheet("color: black;");
            onLogMessage("网络已断开");
        }, Qt::SingleShotConnection);
        return;
    }

    // 连接逻辑
    QString host = ui->edit_ip->text().trimmed();
    quint16 port = ui->edit_port->text().toUShort();
    if (host.isEmpty() || port == 0)
    {
        QMessageBox::warning(this, "错误", "请输入有效的 IP 地址和端口号");
        return;
    }

    m_netThread = new QThread(this);
    m_videoReceiver = new VideoReceiver(host, port);
    m_videoReceiver->moveToThread(m_netThread);

    connect(m_netThread, &QThread::started, m_videoReceiver, &VideoReceiver::start);
    connect(m_netThread, &QThread::finished, m_videoReceiver, &QObject::deleteLater);

    connect(m_videoReceiver, &VideoReceiver::connected, this, &MainWindow::onNetConnected, Qt::QueuedConnection);
    connect(m_videoReceiver, &VideoReceiver::frameReceived, this, &MainWindow::onFrameReceived, Qt::QueuedConnection);
    connect(m_videoReceiver, &VideoReceiver::logMessage, this, &MainWindow::onLogMessage, Qt::QueuedConnection);

    m_netThread->start();
}

// 【完全简化】按钮仅单一功能：点击一次下发一次单帧推理指令 0x0010
void MainWindow::on_btn_action_clicked()
{
    if (!m_serialWorker) {
        onLogMessage("串口未连接，无法执行推理");
        return;
    }
    // 固定下发单帧拍照推理指令
    m_serialWorker->sendCommand(0x0010);
    onLogMessage("下发指令：单帧拍照推理 CMD=0x0010");
}

// ---------- 工作线程信号处理槽 ----------
void MainWindow::onSerialConnected(bool success)
{
    if (success) {
        ui->label_serial_status->setText("已连接");
        ui->label_serial_status->setStyleSheet("color: green;");
        ui->btn_serial->setText("断开串口");
        ui->combo_port->setEnabled(false);
        ui->combo_baud->setEnabled(false);
        ui->btn_action->setEnabled(true);
        m_isVideoPlaying = true;

        // 【NEW】串口连接成功，自动下发 0x0001 自动播放视频
        m_serialWorker->sendCommand(0x0001);
        onLogMessage("串口连接成功，自动下发播放视频指令 CMD=0x0001");
    } else {
        ui->label_serial_status->setText("连接失败");
        ui->label_serial_status->setStyleSheet("color: red;");
        if (m_serialThread && !m_serialThread->isRunning()) {
            ui->btn_serial->setText("连接串口");
            ui->combo_port->setEnabled(true);
            ui->combo_baud->setEnabled(true);
            ui->btn_action->setEnabled(false);
            m_isVideoPlaying = false;
        }
    }
}

void MainWindow::onNetConnected(bool success)
{
    if (success) {
        ui->label_net_status->setText("已连接");
        ui->label_net_status->setStyleSheet("color: green;");
        ui->btn_net->setText("断开网络");
    } else {
        ui->label_net_status->setText("未连接");
        ui->label_net_status->setStyleSheet("color: red;");
        ui->btn_net->setText("连接网络");
    }
}

void MainWindow::onFrameReceived(const QImage &image)
{
    m_currentFrame = image;
}

void MainWindow::onResultReceived(const QString &text)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    ui->text_result->append(QString("[%1] %2").arg(timestamp).arg(text));
    // 单帧推理完成无需切换按钮，按钮始终可用、文字不变
}

// 结构化上行帧解析槽
void MainWindow::onInferenceResultReceived(quint16 echoCmd, quint8 tongueCode, quint8 confRaw, int confPercent, const QString &typeName)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString detailLog = QString("[%1] 原始上行帧解析：回显=0x%2 | 舌苔编码=0x%3 | 置信度原始字节=0x%4 | 置信度=%5% | 舌苔类型=%6")
                            .arg(timestamp)
                            .arg(QString::number(echoCmd, 16).toUpper())
                            .arg(QString::number(tongueCode, 16).toUpper())
                            .arg(QString::number(confRaw, 16).toUpper())
                            .arg(confPercent)
                            .arg(typeName);
    // ui->text_result->append(detailLog);
    qDebug() << detailLog;
}

void MainWindow::onLogMessage(const QString &msg)
{
    qDebug() << msg;
}

void MainWindow::updateVideoDisplay()
{
    if (m_currentFrame.isNull()) {
        ui->label_video->setText("等待视频流...");
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(m_currentFrame);
    QSize labelSize = ui->label_video->size() - QSize(10, 10);
    QPixmap scaled = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_video->setPixmap(scaled);
}

void MainWindow::on_btn_serial_refresh_clicked()
{
    refreshPorts();
}