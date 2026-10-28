#include "VideoReceiver.h"
#include <QDataStream>
#include <QThread>

// 视频流帧头魔数
static const QByteArray NET_MAGIC = "LZDZ";
// 单帧图像最大5MB，防止恶意超大包占内存
#define MAX_IMG_SIZE (5 * 1024 * 1024)

VideoReceiver::VideoReceiver(const QString &host, quint16 port, QObject *parent)
    : QObject(parent),
    m_host(host),
    m_port(port),
    m_running(false),
    m_state(WaitingMagic),
    m_socket(new QTcpSocket(this))
{
    // 绑定socket信号，使用唯一连接避免重复绑定
    connect(m_socket, &QTcpSocket::connected, this, &VideoReceiver::onConnected, Qt::UniqueConnection);
    connect(m_socket, &QTcpSocket::disconnected, this, &VideoReceiver::onDisconnected, Qt::UniqueConnection);
    connect(m_socket, &QTcpSocket::readyRead, this, &VideoReceiver::onReadyRead, Qt::UniqueConnection);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &VideoReceiver::onError, Qt::UniqueConnection);
}

VideoReceiver::~VideoReceiver()
{
    stop();
}

// 发起TCP连接
void VideoReceiver::start()
{
    m_running = true;
    // 重置解析状态机与接收缓存，防止残留脏数据
    m_state = WaitingMagic;
    m_rxBuffer.clear();
    m_socket->connectToHost(m_host, m_port);
    emit logMessage(tr("[网络] 正在连接视频服务器 %1:%2").arg(m_host).arg(m_port));
}

// 停止接收、关闭socket、关闭自动重连（关键修复）
void VideoReceiver::stop()
{
    m_running = false;       // 关闭重连开关
    m_state = WaitingMagic;  // 重置解析状态
    m_rxBuffer.clear();      // 清空缓存脏数据

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->disconnectFromHost();
    }
    m_socket->close();
    emit connected(false);
    emit logMessage(tr("[网络] 手动断开视频流，停止自动重连"));
}

// TCP连接成功回调
void VideoReceiver::onConnected()
{
    emit connected(true);
    emit logMessage(tr("[网络] 视频服务器连接成功"));
}

// TCP断开回调
void VideoReceiver::onDisconnected()
{
    emit connected(false);
    emit logMessage(tr("[网络] 视频流断开"));

    // 只有 m_running=true 才自动重连；手动stop后不会重连
    if (m_running)
    {
        emit logMessage(tr("[网络] 2秒后自动重连视频流..."));
        QTimer::singleShot(2000, this, &VideoReceiver::start);
    }
}

// 收到TCP数据
void VideoReceiver::onReadyRead()
{
    m_rxBuffer.append(m_socket->readAll());
    parseFrame();
}

// Socket异常报错
void VideoReceiver::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit logMessage(tr("[网络错误] %1").arg(m_socket->errorString()));
    emit connected(false);
}

// 从缓存读取指定长度字节，不足返回false
bool VideoReceiver::readAllBytes(QByteArray &buffer, qint64 size)
{
    if (m_rxBuffer.size() < size)
        return false;
    buffer = m_rxBuffer.left(size);
    m_rxBuffer.remove(0, size);
    return true;
}

// 状态机解析完整视频帧
void VideoReceiver::parseFrame()
{
    while (true)
    {
        switch (m_state)
        {
        case WaitingMagic:
        {
            // 查找帧魔数 LZDZ
            int magicIdx = m_rxBuffer.indexOf(NET_MAGIC);
            if (magicIdx == -1)
            {
                // 保留末尾3字节，防止魔数被分包截断丢失
                if (m_rxBuffer.size() > NET_MAGIC.size() - 1)
                    m_rxBuffer = m_rxBuffer.right(NET_MAGIC.size() - 1);
                return;
            }
            // 丢弃魔数前无效垃圾数据
            if (magicIdx > 0)
                m_rxBuffer.remove(0, magicIdx);
            // 移除4字节魔数
            m_rxBuffer.remove(0, NET_MAGIC.size());
            m_state = WaitingLen;
            break;
        }
        case WaitingLen:
        {
            QByteArray lenBuf;
            if (!readAllBytes(lenBuf, 4)) return;
            QDataStream ds(lenBuf);
            ds.setByteOrder(QDataStream::BigEndian);
            ds >> m_dataLen;

            // 校验图像长度，超大包直接丢弃防内存溢出
            if (m_dataLen > MAX_IMG_SIZE || m_dataLen == 0)
            {
                emit logMessage(tr("[网络] 图像长度非法，丢弃数据包"));
                m_state = WaitingMagic;
                m_rxBuffer.clear();
                break;
            }
            m_state = WaitingRows;
            break;
        }
        case WaitingRows:
        {
            QByteArray rowBuf;
            if (!readAllBytes(rowBuf, 4)) return;
            QDataStream ds(rowBuf);
            ds.setByteOrder(QDataStream::BigEndian);
            ds >> m_rows;
            m_state = WaitingCols;
            break;
        }
        case WaitingCols:
        {
            QByteArray colBuf;
            if (!readAllBytes(colBuf, 4)) return;
            QDataStream ds(colBuf);
            ds.setByteOrder(QDataStream::BigEndian);
            ds >> m_cols;
            m_state = WaitingData;
            break;
        }
        case WaitingData:
        {
            // 图像数据未收齐，等待下次readyRead
            if ((quint32)m_rxBuffer.size() < m_dataLen)
                return;

            QByteArray imgData = m_rxBuffer.left(m_dataLen);
            m_rxBuffer.remove(0, m_dataLen);
            m_state = WaitingMagic;

            QImage img;
            quint32 pixelCount = m_rows * m_cols;
            if (m_dataLen == pixelCount * 3)
            {
                // 先用RGB格式加载，再交换红蓝通道转BGR
                QImage tempImg(reinterpret_cast<const uchar*>(imgData.constData()),
                               m_cols, m_rows, QImage::Format_RGB888);
                // 交换红蓝通道，修正BGR数据流
                img = tempImg.rgbSwapped().copy();
            }
            else if (m_dataLen == pixelCount * 1)
            {
                // 灰度图，拷贝内存
                img = QImage(reinterpret_cast<const uchar*>(imgData.constData()),
                             m_cols, m_rows, QImage::Format_Grayscale8).copy();
            }
            else
            {
                emit logMessage(tr("[网络] 不支持的图像数据长度，丢弃当前帧"));
                break;
            }

            if (!img.isNull())
            {
                emit frameReceived(img);
            }
            break;
        }
        }
    }
}