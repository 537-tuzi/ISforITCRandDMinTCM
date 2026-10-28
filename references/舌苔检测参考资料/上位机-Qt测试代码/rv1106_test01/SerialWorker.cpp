#include "SerialWorker.h"
#include <QThread>
#include <QDataStream>
#include <QDebug>

// ===================== 下行协议宏（PC→RV1106，原有） =====================
#define SERIAL_HEAD_DOWN 0x5A    // PC下发帧头
#define SERIAL_TAIL      0xA5    // 上下行共用帧尾
#define DOWN_FRAME_LEN   4       // 下行固定4字节

// ===================== 【NEW】上行协议宏（RV1106→PC，新增） =====================
#define SERIAL_HEAD_UP   0x5B    // RV1106上行帧头，区分下行0x5A
#define UP_FRAME_LEN     6       // 上行响应固定6字节
#define CONF_SCALE_MAX   0x64    // 置信度最大值0x64=100%

// 【NEW】舌苔编码映射文本，用于日志展示
static QString getTongueTypeName(quint8 code)
{
    switch (code)
    {
    case 0x00: return "灰黑苔";
    case 0x01: return "镜面舌";
    case 0x02: return "薄白苔";
    case 0x03: return "白腻苔";
    case 0x04: return "黄腻苔";
    case 0xFF: return "未识别/异常";
    default: return "未知类型";
    }
}

SerialWorker::SerialWorker(const QString &portName, qint32 baudRate, QObject *parent)
    : QObject(parent), m_portName(portName), m_baudRate(baudRate), m_running(false)
{
    m_serial = new QSerialPort(this);
}

SerialWorker::~SerialWorker()
{
    stop();
}

void SerialWorker::start()
{
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_running = true;
        connect(m_serial, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
        emit connected(true);
        emit logMessage(tr("串口已连接: %1 @ %2").arg(m_portName).arg(m_baudRate));
    } else {
        emit connected(false);
        emit logMessage(tr("串口打开失败: %1").arg(m_serial->errorString()));
    }
}

void SerialWorker::stop()
{
    m_running = false;
    if (m_serial) {
        disconnect(m_serial, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
        if (m_serial->isOpen()) {
            m_serial->close();
        }
    }
    // 【NEW】停止时清空接收缓存，防止残留脏数据
    m_rxBuffer.clear();

    emit connected(false);
    emit logMessage(tr("串口已断开"));
}

void SerialWorker::sendCommand(quint16 cmd)
{
    if (!m_serial || !m_serial->isOpen()) {
        emit logMessage(tr("串口未连接，命令发送失败"));
        return;
    }
    QByteArray frame = buildFrame(cmd);
    qint64 written = m_serial->write(frame);
    if (written == frame.size()) {
        emit logMessage(tr("[→] 下发4字节帧 HEX: %1").arg(frame.toHex().toUpper()));
    } else {
        emit logMessage(tr("[✗] 下发命令发送失败"));
    }
}

void SerialWorker::handleReadyRead()
{
    QByteArray data = m_serial->readAll();
    m_rxBuffer.append(data);
    processBuffer();
}

// 【MODIFY】完全重写：二进制上行6字节帧解析，删除旧的\n文本行逻辑
void SerialWorker::processBuffer()
{
    // 循环解析缓存中完整上行帧
    while (m_rxBuffer.size() >= UP_FRAME_LEN)
    {
        // 1. 查找上行帧头 0x5B
        int headPos = m_rxBuffer.indexOf(static_cast<char>(SERIAL_HEAD_UP));
        if (headPos == -1)
        {
            // 无帧头，清空缓存避免无限堆积
            m_rxBuffer.clear();
            break;
        }

        // 丢弃帧头之前的干扰杂字节
        if (headPos > 0)
        {
            m_rxBuffer.remove(0, headPos);
        }

        // 剩余数据不足6字节，等待下一次接收
        if (m_rxBuffer.size() < UP_FRAME_LEN)
            break;

        // 截取当前候选完整6字节帧
        QByteArray frame = m_rxBuffer.left(UP_FRAME_LEN);
        quint8 frameTail = static_cast<quint8>(frame[5]);

        // 校验帧尾 0xA5，合法帧则解析，否则丢弃1字节重新查找帧头
        if (frameTail != SERIAL_TAIL)
        {
            m_rxBuffer.remove(0, 1);
            continue;
        }

        // 移除已解析完成的6字节数据
        m_rxBuffer.remove(0, UP_FRAME_LEN);

        // ===================== 解析6字节上行帧 =====================
        // 帧结构：0:0x5B(帧头) | 1:CMD_H | 2:CMD_L | 3:舌苔编码 | 4:置信度(0~0x64) |5:0xA5(帧尾)
        quint16 echoCmd = (static_cast<quint8>(frame[1]) << 8) | static_cast<quint8>(frame[2]);
        quint8 tongueCode = static_cast<quint8>(frame[3]);
        quint8 confRaw = static_cast<quint8>(frame[4]);
        int confPercent = qRound((confRaw * 100.0) / CONF_SCALE_MAX);

        // 打印十六进制原始帧日志
        QString hexStr = frame.toHex(' ').toUpper();
        QString typeName = getTongueTypeName(tongueCode);
        emit logMessage(tr("[←] 上行6字节响应帧 HEX: %1").arg(hexStr));
        emit logMessage(tr("    回显指令:0x%1 | 舌苔类型:%2 | 置信度:%3%")
                            .arg(QString::number(echoCmd, 16).toUpper(), typeName, QString::number(confPercent)));

        // 【NEW】结构化信号对外输出解析结果（需要在.h头文件声明信号）
        emit inferenceResultReceived(echoCmd, tongueCode, confRaw, confPercent, typeName);

        // 兼容原有文本信号（可选，若UI旧接口依赖resultReceived）
        QString textResult = tr("CMD回显:0x%1，舌苔:%2，置信度:%3%")
                                 .arg(QString::number(echoCmd,16), typeName, QString::number(confPercent));
        emit resultReceived(textResult);
    }

    // 缓存超限保护，防止内存溢出
    if (m_rxBuffer.size() > 2048)
    {
        m_rxBuffer.clear();
        emit logMessage(tr("[警告] 接收缓存溢出，已清空脏数据"));
    }
}

// 下发帧构造函数（PC→RV1106 4字节帧，原有逻辑不变）
QByteArray SerialWorker::buildFrame(quint16 cmd)
{
    QByteArray frame;
    frame.append(static_cast<char>(SERIAL_HEAD_DOWN));
    // 大端序写入命令高8位、低8位
    frame.append(static_cast<char>((cmd >> 8) & 0xFF));
    frame.append(static_cast<char>(cmd & 0xFF));
    frame.append(static_cast<char>(SERIAL_TAIL));
    return frame;
}