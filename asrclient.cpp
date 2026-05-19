#include "asrclient.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QThread>
#include <QTimer>
#include <QtEndian>
#include <QDebug>

namespace {
constexpr quint8 kMsgStatus = 100;
constexpr quint8 kMsgResult = 101;
constexpr quint8 kMsgAudio = 2;
constexpr quint8 kMsgReset = 3;
constexpr quint32 kMaxFrameSize = 16 * 1024 * 1024;
}

AsrClient& AsrClient::instance()
{
    static AsrClient client;
    return client;
}

AsrClient::AsrClient(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<qint64>("qint64");
    connect(&m_socket, &QTcpSocket::connected, this, &AsrClient::onConnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &AsrClient::onReadyRead);
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onProcessFinished(int,QProcess::ExitStatus)));
}

AsrClient::~AsrClient()
{
    m_socket.disconnectFromHost();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(1500)) {
            m_process.kill();
            m_process.waitForFinished(1500);
        }
    }
}

void AsrClient::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        ensureStarted();
    }
}

void AsrClient::setCurrentUserId(int userId)
{
    m_currentUserId = userId;
}

void AsrClient::submitPcm48k(int userId, const qint16* samples, int sampleCount)
{
    if (!m_enabled || !samples || sampleCount < 3) {
        return;
    }

    if (userId <= 0) {
        userId = m_currentUserId;
    }
    if (userId <= 0) {
        return;
    }

    QByteArray pcm16k;
    pcm16k.resize((sampleCount / 3) * static_cast<int>(sizeof(qint16)));
    qint16* out = reinterpret_cast<qint16*>(pcm16k.data());
    int outIndex = 0;
    for (int i = 0; i + 2 < sampleCount; i += 3) {
        const int avg = (static_cast<int>(samples[i]) +
                         static_cast<int>(samples[i + 1]) +
                         static_cast<int>(samples[i + 2])) / 3;
        out[outIndex++] = static_cast<qint16>(avg);
    }
    pcm16k.resize(outIndex * static_cast<int>(sizeof(qint16)));

    QMetaObject::invokeMethod(this, "sendPcm16k", Qt::QueuedConnection,
                              Q_ARG(int, userId),
                              Q_ARG(QByteArray, pcm16k),
                              Q_ARG(qint64, QDateTime::currentMSecsSinceEpoch()));
}

void AsrClient::resetUser(int userId)
{
    if (!m_enabled || userId <= 0) {
        return;
    }
    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << static_cast<qint32>(userId);
    sendFrame(kMsgReset, payload);
}

void AsrClient::sendPcm16k(int userId, QByteArray pcm16k, qint64 timestamp)
{
    if (!m_enabled || pcm16k.isEmpty()) {
        return;
    }
    ensureStarted();
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_12);
    out << static_cast<qint32>(userId) << timestamp << pcm16k;
    sendFrame(kMsgAudio, payload);
}

void AsrClient::ensureStarted()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState ||
        m_socket.state() == QAbstractSocket::ConnectingState) {
        return;
    }
    if (m_starting) {
        return;
    }

    const QString exe = helperPath();
    if (!QFileInfo::exists(exe)) {
        setStatus(false, QString("ASR helper not found: %1").arg(exe));
        return;
    }

    m_starting = true;
    m_port = static_cast<quint16>(35000 + QRandomGenerator::global()->bounded(20000));
    QStringList args;
    args << "--port" << QString::number(m_port)
         << "--model-dir" << modelDir()
         << "--lib-dir" << libDir();

    if (m_process.state() == QProcess::NotRunning) {
        m_process.setProgram(exe);
        m_process.setArguments(args);
        m_process.setWorkingDirectory(QFileInfo(exe).absolutePath());
        m_process.start();
    }

    QTimer::singleShot(600, this, [this]() {
        if (!m_enabled) {
            m_starting = false;
            return;
        }
        m_socket.connectToHost(QHostAddress::LocalHost, m_port);
    });
}

void AsrClient::sendFrame(quint8 type, const QByteArray& payload)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }
    const quint32 size = static_cast<quint32>(1 + payload.size());
    QByteArray frame;
    frame.resize(4 + static_cast<int>(size));
    qToLittleEndian(size, reinterpret_cast<uchar*>(frame.data()));
    frame[4] = static_cast<char>(type);
    if (!payload.isEmpty()) {
        memcpy(frame.data() + 5, payload.constData(), payload.size());
    }
    m_socket.write(frame);
}

void AsrClient::onConnected()
{
    m_starting = false;
    setStatus(true, "ASR helper connected");
}

void AsrClient::onReadyRead()
{
    m_readBuffer.append(m_socket.readAll());
    parseFrames();
}

void AsrClient::parseFrames()
{
    while (m_readBuffer.size() >= 4) {
        const quint32 size = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(m_readBuffer.constData()));
        if (size == 0 || size > kMaxFrameSize) {
            m_readBuffer.clear();
            setStatus(false, "Invalid ASR helper frame");
            return;
        }
        if (m_readBuffer.size() < static_cast<int>(4 + size)) {
            return;
        }

        const quint8 type = static_cast<quint8>(m_readBuffer[4]);
        QByteArray payload = m_readBuffer.mid(5, static_cast<int>(size - 1));
        m_readBuffer.remove(0, static_cast<int>(4 + size));

        QDataStream in(payload);
        in.setVersion(QDataStream::Qt_5_12);
        if (type == kMsgStatus) {
            bool ok = false;
            QString message;
            in >> ok >> message;
            setStatus(ok, message);
        } else if (type == kMsgResult) {
            qint32 userId = 0;
            bool isFinal = false;
            qint64 timestamp = 0;
            QString text;
            in >> userId >> isFinal >> timestamp >> text;
            if (!text.trimmed().isEmpty()) {
                emit textRecognized(userId, text.trimmed(), isFinal, timestamp);
            }
        }
    }
}

void AsrClient::onSocketError(QAbstractSocket::SocketError)
{
    m_starting = false;
    if (m_enabled) {
        setStatus(false, m_socket.errorString());
    }
}

void AsrClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    m_starting = false;
    m_available = false;
    emit statusChanged(false, QString("ASR helper exited: %1").arg(exitCode));
}

void AsrClient::setStatus(bool available, const QString& message)
{
    if (m_available == available && message.isEmpty()) {
        return;
    }
    m_available = available;
    qDebug() << "ASR:" << available << message;
    emit statusChanged(available, message);
}


QString AsrClient::helperPath() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString deployed = appDir.filePath("asr/sherpa_asr_helper.exe");
    if (QFileInfo::exists(deployed)) return deployed;
    const QString src = appDir.absoluteFilePath("../../Wechat/release/asr/sherpa_asr_helper.exe");
    return QDir::cleanPath(src);
}

QString AsrClient::modelDir() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString deployed = appDir.filePath("asr/model");
    if (QFileInfo::exists(deployed)) return deployed;
    const QString src = appDir.absoluteFilePath("../../Wechat/sherpa-onnx-streaming-paraformer-bilingual-zh-en");
    if (QFileInfo::exists(src)) return QDir::cleanPath(src);
    return appDir.filePath("../sherpa-onnx-streaming-paraformer-bilingual-zh-en");
}

QString AsrClient::libDir() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString deployed = appDir.filePath("asr/lib");
    if (QFileInfo::exists(deployed)) return deployed;
    const QString src = appDir.absoluteFilePath("../../Wechat/sherpaApi/lib");
    if (QFileInfo::exists(src)) return QDir::cleanPath(src);
    return appDir.filePath("../sherpaApi/lib");
}
