#include "TcpClient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::disconnected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketError);
#else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpClient::onSocketError);
#endif
}

TcpClient::~TcpClient()
{
    disconnectFromHost();
}

bool TcpClient::connectToHost(const QString &host, quint16 port, int timeoutMs)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }

    m_socket->connectToHost(host, port);
    return m_socket->waitForConnected(timeoutMs);
}

void TcpClient::disconnectFromHost()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
}

bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

qint64 TcpClient::sendData(const QByteArray &data)
{
    if (!isConnected()) {
        emit errorOccurred("未连接到服务器");
        return -1;
    }
    return m_socket->write(data);
}

qint64 TcpClient::sendData(const char *data, int len)
{
    return sendData(QByteArray(data, len));
}

QByteArray TcpClient::readAll()
{
    return m_socket->readAll();
}

bool TcpClient::waitForReadyRead(int msecs)
{
    return m_socket->waitForReadyRead(msecs);
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void TcpClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(m_socket->errorString());
}