#include "MainWindow.h"
#include "TcpClient.h"
#include "FileUploadTask.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tcpClient(new TcpClient(this))
    , m_uploadTask(new FileUploadTask(m_tcpClient, this))
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    // 服务器设置
    auto *serverLayout = new QHBoxLayout;
    m_hostEdit = new QLineEdit("192.168.255.129");
    m_portEdit = new QLineEdit("10086");
    m_portEdit->setMaximumWidth(80);
    serverLayout->addWidget(new QLabel("服务器:"));
    serverLayout->addWidget(m_hostEdit);
    serverLayout->addWidget(new QLabel("端口:"));
    serverLayout->addWidget(m_portEdit);
    serverLayout->addStretch();

    // 文件选择
    auto *fileLayout = new QHBoxLayout;
    m_fileEdit = new QLineEdit;
    m_fileEdit->setReadOnly(true);
    m_selectBtn = new QPushButton("选择文件");
    fileLayout->addWidget(m_fileEdit);
    fileLayout->addWidget(m_selectBtn);

    // 进度显示
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_chunkLabel = new QLabel("分片: 0/0");
    m_statusLabel = new QLabel("就绪");

    // 操作按钮
    auto *btnLayout = new QHBoxLayout;
    m_uploadBtn = new QPushButton("开始上传");
    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setEnabled(false);
    btnLayout->addWidget(m_uploadBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(serverLayout);
    mainLayout->addLayout(fileLayout);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(m_chunkLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();

    setCentralWidget(central);
    setWindowTitle("文件上传客户端");
    resize(500, 250);

    // 界面信号
    connect(m_selectBtn, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    connect(m_uploadBtn, &QPushButton::clicked, this, &MainWindow::onUpload);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancel);

    // 上传任务信号
    connect(m_uploadTask, &FileUploadTask::uploadProgress, this, &MainWindow::onProgress);
    connect(m_uploadTask, &FileUploadTask::chunkSent, this, &MainWindow::onChunkSent);
    connect(m_uploadTask, &FileUploadTask::uploadFinished, this, &MainWindow::onFinished);
    connect(m_uploadTask, &FileUploadTask::errorOccurred, this, &MainWindow::onError);
}

MainWindow::~MainWindow() = default;

void MainWindow::onSelectFile()
{
    QString path = QFileDialog::getOpenFileName(this, "选择要上传的文件");
    if (!path.isEmpty()) {
        m_fileEdit->setText(path);
    }
}

void MainWindow::onUpload()
{
    if (m_fileEdit->text().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择文件");
        return;
    }

    m_uploadTask->setFilePath(m_fileEdit->text());
    m_uploadTask->setServer(m_hostEdit->text(), m_portEdit->text().toUShort());
    m_uploadTask->start();

    m_uploadBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_statusLabel->setText("正在上传...");
}

void MainWindow::onCancel()
{
    m_uploadTask->cancel();
    m_uploadBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_statusLabel->setText("已取消");
}

void MainWindow::onProgress(qint64 sent, qint64 total, int percent)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(QString("已发送: %1 / %2 bytes (%3%)")
                               .arg(sent).arg(total).arg(percent));
}

void MainWindow::onChunkSent(int current, int total)
{
    m_chunkLabel->setText(QString("分片: %1/%2").arg(current).arg(total));
}

void MainWindow::onFinished(bool success, const QString &msg)
{
    m_uploadBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);

    if (success) {
        m_statusLabel->setText("上传完成");
        QMessageBox::information(this, "成功", msg);
    } else {
        m_statusLabel->setText("上传失败");
        QMessageBox::critical(this, "失败", msg);
    }
}

void MainWindow::onError(const QString &error)
{
    m_statusLabel->setText("错误: " + error);
}