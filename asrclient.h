#ifndef ASRCLIENT_H
#define ASRCLIENT_H

#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QByteArray>
#include <QMutex>

class AsrClient : public QObject
{
    Q_OBJECT
public:
    static AsrClient& instance();

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    bool isAvailable() const { return m_available; }

    void setCurrentUserId(int userId);
    void submitPcm48k(int userId, const qint16* samples, int sampleCount);
    void resetUser(int userId);

signals:
    void textRecognized(int userId, const QString& text, bool isFinal, qint64 timestamp);
    void statusChanged(bool available, const QString& message);

private slots:
    void sendPcm16k(int userId, QByteArray pcm16k, qint64 timestamp);
    void onConnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    explicit AsrClient(QObject* parent = nullptr);
    ~AsrClient();
    AsrClient(const AsrClient&) = delete;
    AsrClient& operator=(const AsrClient&) = delete;

    void ensureStarted();
    void sendFrame(quint8 type, const QByteArray& payload);
    void parseFrames();
    void setStatus(bool available, const QString& message);
    QString helperPath() const;
    QString modelDir() const;
    QString libDir() const;

private:
    QProcess m_process;
    QTcpSocket m_socket;
    QByteArray m_readBuffer;
    bool m_enabled = false;
    bool m_available = false;
    bool m_starting = false;
    quint16 m_port = 0;
    int m_currentUserId = 0;
    mutable QMutex m_submitMutex;
};

#endif // ASRCLIENT_H
