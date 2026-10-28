#include "FileUploadTask.h"
#include <QFileInfo>
#include <QDebug>

FileUploadTask::FileUploadTask(TcpClient *tcpClient, QObject *parent)
    : QObject(parent)
    , m_tcpClient(tcpClient)
{
    // 如果外部没传入TcpClient，自己创建一个
    if (!m_tcpClient) {
        m_tcpClient = new TcpClient(this);
        m_ownTcpClient = true;
    }

    connect(m_tcpClient, &TcpClient::connected, this, &FileUploadTask::onTcpConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &FileUploadTask::onTcpDisconnected);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &FileUploadTask::onTcpError);
    connect(m_tcpClient, &TcpClient::dataReceived, this, &FileUploadTask::onTcpDataReceived);


}

FileUploadTask::~FileUploadTask()
{
    cancel();
}

void FileUploadTask::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

void FileUploadTask::setServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void FileUploadTask::start()
{
    if (m_running) {
        emit errorOccurred("已有上传任务正在进行");
        return;
    }

    if (m_filePath.isEmpty()) {
        emit errorOccurred("未设置文件路径");
        return;
    }

    QFileInfo fileInfo(m_filePath);
    if (!fileInfo.exists()) {
        emit errorOccurred("文件不存在: " + m_filePath);
        return;
    }
    if (!fileInfo.isReadable()) {
        emit errorOccurred("文件不可读: " + m_filePath);
        return;
    }

    m_fileSize = fileInfo.size();
    m_totalChunks = (m_fileSize + CHUNK_DATA_SIZE - 1) / CHUNK_DATA_SIZE;
    if (m_totalChunks == 0) m_totalChunks = 1;

    m_currentChunk = 0;
    m_sentBytes = 0;
    m_recvBuffer.clear();

    // 打开文件
    if (m_file) {
        delete m_file;
    }
    m_file = new QFile(m_filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法打开文件: " + m_file->errorString());
        delete m_file;
        m_file = nullptr;
        return;
    }

    m_running = true;
    emit uploadStarted();

    // 连接服务器
    if (!connectServer()) {
        m_running = false;
        emit uploadFinished(false, "连接服务器失败");
        cleanup();
        return;
    }
}

void FileUploadTask::cancel()
{
    if (!m_running) return;

    m_running = false;

    if (m_file) {
        m_file->close();
    }

    if (m_ownTcpClient) {
        m_tcpClient->disconnectFromHost();
    }

    cleanup();
}

bool FileUploadTask::isRunning() const
{
    return m_running;
}

bool FileUploadTask::connectServer()
{
    if (m_tcpClient->isConnected()) {
        onTcpConnected();
        return true;
    }
    return m_tcpClient->connectToHost(m_host, m_port, 5000);
}

void FileUploadTask::sendAllChunks()
{
    // 连续发送所有分片，不等待ACK
    for (int i = 0; i < m_totalChunks; ++i) {
        if (!m_running) break;

        if (!sendSingleChunk(i)) {
            m_running = false;
            emit uploadFinished(false, QString("发送分片 %1 失败").arg(i));
            cleanup();
            return;
        }

        // 每发送一个分片，更新进度
        emit chunkSent(i + 1, m_totalChunks);
        emit uploadProgress(m_sentBytes, m_fileSize,
                            static_cast<int>((m_sentBytes * 100) / m_fileSize));

        // 小间隔，避免瞬间发送太快导致网络拥塞或服务器处理不过来
        // 如果不需要间隔，可以注释掉下面这行
        // QThread::msleep(100);
    }

    if (m_running) {
        // 所有分片发送完毕
        m_running = false;
        emit uploadProgress(m_fileSize, m_fileSize, 100);
        emit uploadFinished(true, "上传完成");
        cleanup();
    }
}

bool FileUploadTask::sendSingleChunk(int index)
{
    FILE_UPLOAD_REQ req;
    int actualSize = 0;

    if (!buildChunkRequest(index, req, actualSize)) {
        return false;
    }

    // 构建完整数据包
    QByteArray packet;
    packet.reserve(sizeof(HEAD) + actualSize);

    HEAD head;
    head.type = FILE_UPLOAD;
    head.len = sizeof(HEAD) + actualSize;
    head.len = sizeof(HEAD) + actualSize;

    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), actualSize);

    qint64 written = m_tcpClient->sendData(packet);
    if (written < 0) {
        return false;
    }
    qDebug() << "发送分片" << index + 1 << "/" << m_totalChunks
             << "大小:" << req.chunkSize << "bytes";

    return true;
}

bool FileUploadTask::buildChunkRequest(int chunkIndex, FILE_UPLOAD_REQ &req, int &actualSize)
{
    memset(&req, 0, sizeof(FILE_UPLOAD_REQ));

    QFileInfo fileInfo(m_filePath);
    QString baseName = fileInfo.fileName();
    QByteArray nameBytes = baseName.toUtf8();

    if (nameBytes.size() >= 256) {
        emit errorOccurred("文件名过长");
        return false;
    }

    memcpy(req.filename, nameBytes.constData(), nameBytes.size());
    req.filename[nameBytes.size()] = '\0';

    req.chunkTotal = m_totalChunks;
    req.chunkIndex = chunkIndex;

    qint64 offset = static_cast<qint64>(chunkIndex) * CHUNK_DATA_SIZE;
    if (!m_file->seek(offset)) {
        return false;
    }

    qint64 bytesRead = m_file->read(req.chunkData, CHUNK_DATA_SIZE);
    if (bytesRead < 0) {
        return false;
    }

    req.chunkSize = static_cast<int>(bytesRead);
    m_sentBytes += bytesRead;

    actualSize = sizeof(req.filename) + sizeof(req.chunkTotal) +
                 sizeof(req.chunkIndex) + sizeof(req.chunkSize) + req.chunkSize;

    return true;
}

void FileUploadTask::cleanup()
{
    if (m_file) {
        delete m_file;
        m_file = nullptr;
    }
    m_connected = false;
    m_currentChunk = 0;
    m_totalChunks = 0;
    m_fileSize = 0;
    m_sentBytes = 0;
}

void FileUploadTask::onTcpConnected()
{
    qDebug() << "服务器连接成功，开始连续发送分片";
    m_connected = true;
    sendAllChunks();
}

void FileUploadTask::onTcpDisconnected()
{
    qDebug() << "与服务器断开连接";
    m_connected = false;
    if (m_running) {
        m_running = false;
        emit uploadFinished(false, "连接意外断开");
        cleanup();
    }
}

void FileUploadTask::onTcpError(const QString &error)
{
    qDebug() << "TCP错误:" << error;
    if (m_running) {
        m_running = false;
        emit uploadFinished(false, "网络错误: " + error);
        cleanup();
    }
}

void FileUploadTask::onTcpDataReceived(const QByteArray &data)
{
    // 非阻塞模式下，可以在这里处理服务器的异步响应
    // 当前实现是连续发送，不依赖ACK，但保留接收能力用于日志或后续扩展
    m_recvBuffer.append(data);

    // 示例：解析服务器返回的状态（假设每个ACK是4字节int）
    while (m_recvBuffer.size() >= 4) {
        int status = *reinterpret_cast<const int*>(m_recvBuffer.constData());
        m_recvBuffer.remove(0, 4);
        qDebug() << "收到服务器响应，状态码:" << status;
    }
}
