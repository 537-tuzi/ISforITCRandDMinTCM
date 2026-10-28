#include "fileuploader.h"

#include <QFileInfo>
#include <QDebug>

FileUploader::FileUploader(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_retryTimer(new QTimer(this))
    , m_file(nullptr)
{
    connect(m_socket, &QTcpSocket::connected, this, &FileUploader::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &FileUploader::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &FileUploader::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &FileUploader::onError);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &FileUploader::onBytesWritten);

    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &FileUploader::onRetryTimeout);
}

FileUploader::~FileUploader()
{
    cancelUpload();
    if (m_file) {
        delete m_file;
    }
}

void FileUploader::setServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
}

void FileUploader::uploadFile(const QString &filePath)
{
    if (m_uploading) {
        emit errorOccurred("已有上传任务正在进行");
        return;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit errorOccurred("文件不存在: " + filePath);
        return;
    }

    if (!fileInfo.isReadable()) {
        emit errorOccurred("文件不可读: " + filePath);
        return;
    }

    m_filePath = filePath;
    m_fileSize = fileInfo.size();

    // 计算总分片数
    m_totalChunks = (m_fileSize + CHUNK_DATA_SIZE - 1) / CHUNK_DATA_SIZE;
    if (m_totalChunks == 0) m_totalChunks = 1; // 空文件也传1个分片

    m_currentChunk = 0;
    m_sentBytes = 0;
    m_retryCount = 0;
    m_pendingData.clear();

    // 打开文件
    if (m_file) {
        delete m_file;
    }
    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法打开文件: " + m_file->errorString());
        delete m_file;
        m_file = nullptr;
        return;
    }

    m_uploading = true;

    // 连接服务器
    m_socket->connectToHost(m_host, m_port);
}

void FileUploader::cancelUpload()
{
    m_retryTimer->stop();
    m_uploading = false;
    m_waitingForAck = false;

    if (m_file) {
        m_file->close();
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    resetState();
}

void FileUploader::onConnected()
{
    qDebug() << "已连接到服务器:" << m_host << ":" << m_port;
    emit connected();

    // 连接成功后立即发送第一个分片
    sendNextChunk();
}

void FileUploader::onDisconnected()
{
    qDebug() << "与服务器断开连接";

    if (m_uploading) {
        // 非预期断开，尝试重连
        scheduleRetry();
    }
    emit disconnected();
}

void FileUploader::onReadyRead()
{
    m_pendingData.append(m_socket->readAll());

    // 处理服务器响应（假设服务器返回简单的ACK：4字节状态码）
    // 根据实际协议调整此处解析逻辑
    while (m_pendingData.size() >= 4) {
        int status = *reinterpret_cast<const int*>(m_pendingData.constData());
        m_pendingData.remove(0, 4);

        if (status == 0) { // 假设0表示成功
            m_waitingForAck = false;
            m_retryCount = 0; // 重置重试计数

            m_currentChunk++;

            emit chunkUploaded(m_currentChunk, m_totalChunks);
            emit uploadProgress(m_sentBytes, m_fileSize,
                                static_cast<int>((m_sentBytes * 100) / m_fileSize));

            // 检查是否完成
            if (m_currentChunk >= m_totalChunks) {
                m_uploading = false;
                m_file->close();
                m_socket->disconnectFromHost();
                emit uploadFinished(true, "上传成功");
                resetState();
                return;
            }

            // 发送下一个分片
            sendNextChunk();
        } else {
            // 服务器返回错误
            m_uploading = false;
            emit uploadFinished(false, QString("服务器返回错误码: %1").arg(status));
            resetState();
        }
    }
}

void FileUploader::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QString errorStr = m_socket->errorString();
    qDebug() << "Socket错误:" << errorStr;

    if (m_uploading) {
        scheduleRetry();
    } else {
        emit errorOccurred(errorStr);
    }
}

void FileUploader::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)
    // 数据已写入OS缓冲区，实际发送进度在收到ACK后更新更可靠
}

void FileUploader::onRetryTimeout()
{
    if (!m_uploading || m_retryCount >= MAX_RETRY_COUNT) {
        m_uploading = false;
        emit uploadFinished(false, "上传失败：超过最大重试次数");
        resetState();
        return;
    }

    m_retryCount++;
    qDebug() << "重试第" << m_retryCount << "次，发送分片" << m_currentChunk;

    // 重新连接并发送当前分片
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->connectToHost(m_host, m_port);
}

void FileUploader::sendNextChunk()
{
    if (!m_uploading || m_waitingForAck) {
        return;
    }

    sendChunk(m_currentChunk);
}

void FileUploader::sendChunk(int index)
{
    if (!m_file || !m_file->isOpen()) {
        emit errorOccurred("文件未打开");
        return;
    }

    FILE_UPLOAD_REQ req;
    int actualSize = 0;

    if (!buildUploadRequest(index, req, actualSize)) {
        emit errorOccurred("构建请求失败");
        return;
    }

    // 构建完整数据包：HEAD + FILE_UPLOAD_REQ
    QByteArray packet;
    packet.reserve(sizeof(HEAD) + actualSize);

    HEAD head;
    head.type = FILE_UPLOAD;
    head.len = sizeof(HEAD) + actualSize;

    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), actualSize);

    m_waitingForAck = true;
    qint64 written = m_socket->write(packet);

    if (written < 0) {
        m_waitingForAck = false;
        scheduleRetry();
        return;
    }

    qDebug() << "发送分片" << index << "/" << m_totalChunks
             << "大小:" << req.chunkSize << "bytes";
}

bool FileUploader::buildUploadRequest(int chunkIndex, FILE_UPLOAD_REQ &req, int &actualSize)
{
    // 清零结构体
    memset(&req, 0, sizeof(FILE_UPLOAD_REQ));

    // 文件名（取basename）
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

    // 读取分片数据
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

    // 计算实际发送大小（不包含未使用的chunkData空间）
    actualSize = sizeof(req.filename) + sizeof(req.chunkTotal) +
                 sizeof(req.chunkIndex) + sizeof(req.chunkSize) + req.chunkSize;

    return true;
}

void FileUploader::resetState()
{
    m_currentChunk = 0;
    m_totalChunks = 0;
    m_fileSize = 0;
    m_sentBytes = 0;
    m_retryCount = 0;
    m_waitingForAck = false;
    m_pendingData.clear();
}

void FileUploader::scheduleRetry()
{
    if (m_retryCount < MAX_RETRY_COUNT) {
        m_retryTimer->start(RETRY_INTERVAL_MS);
    }
}
