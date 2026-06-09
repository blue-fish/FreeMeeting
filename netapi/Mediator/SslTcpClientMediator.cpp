#include "SslTcpClientMediator.h"

#include <QDataStream>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QSslError>

SslTcpClientMediator::SslTcpClientMediator()
    : m_socket(nullptr)
{
    memset(m_szBufIP, 0, sizeof(m_szBufIP));
    m_port = 0;
}

SslTcpClientMediator::~SslTcpClientMediator()
{
    CloseNet();
}

bool SslTcpClientMediator::OpenNet(const char* szBufIP, unsigned short port)
{
    qDebug() << __func__;
    strcpy_s(m_szBufIP, sizeof(m_szBufIP), szBufIP);
    m_port = port;

    qDebug() << "SSL supported:" << QSslSocket::supportsSsl();
    qDebug() << "SSL build version:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "SSL runtime version:" << QSslSocket::sslLibraryVersionString();

    resetSocket();
    m_socket = new QSslSocket(this);
    m_socket->setProxy(QNetworkProxy::NoProxy);

    connect(m_socket, &QSslSocket::readyRead, this, [this]() {
        handleReadyRead();
    });
    connect(m_socket, &QSslSocket::disconnected, this, [this]() {
        Q_EMIT SIG_disConnect();
    });
    connect(m_socket,
            QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this,
            [this](const QList<QSslError>& errors) {
        qDebug() << "SSL errors:" << errors;
        if (m_socket) {
            m_socket->ignoreSslErrors();
        }
    });

    m_socket->connectToHostEncrypted(QString::fromLatin1(szBufIP), port);
    if (!m_socket->waitForEncrypted(5000)) {
        qDebug() << "SSL connect failed:" << m_socket->errorString();
        resetSocket();
        return false;
    }

    qDebug() << "SSL connect success";
    return true;
}

void SslTcpClientMediator::CloseNet()
{
    resetSocket();
}

bool SslTcpClientMediator::SendData(unsigned int lSendIP, char* buf, int nlen)
{
    Q_UNUSED(lSendIP);
    if (!buf || nlen <= 0) {
        return false;
    }

    if (!IsConnected()) {
        resetSocket();
        if (!OpenNet(m_szBufIP, m_port)) {
            return false;
        }
    }

    QByteArray packet;
    packet.resize(static_cast<int>(sizeof(int)) + nlen);
    memcpy(packet.data(), &nlen, sizeof(int));
    memcpy(packet.data() + sizeof(int), buf, nlen);

    qint64 written = m_socket->write(packet);
    if (written != packet.size()) {
        return false;
    }
    return m_socket->flush();
}

void SslTcpClientMediator::DealData(unsigned int lSendIP, char* buf, int nlen)
{
    Q_EMIT SIG_ReadyData(lSendIP, buf, nlen);
}

void SslTcpClientMediator::disConnect()
{
    Q_EMIT SIG_disConnect();
}

bool SslTcpClientMediator::IsConnected()
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState
           && m_socket->isEncrypted();
}

void SslTcpClientMediator::handleReadyRead()
{
    if (!m_socket) {
        return;
    }

    m_recvBuffer.append(m_socket->readAll());

    while (m_recvBuffer.size() >= static_cast<int>(sizeof(int))) {
        int packSize = 0;
        memcpy(&packSize, m_recvBuffer.constData(), sizeof(int));
        if (packSize <= 0 || packSize > 1024 * 1024) {
            qDebug() << "Invalid SSL packet size:" << packSize;
            CloseNet();
            return;
        }

        int frameSize = static_cast<int>(sizeof(int)) + packSize;
        if (m_recvBuffer.size() < frameSize) {
            return;
        }

        char* payload = new char[packSize];
        memcpy(payload, m_recvBuffer.constData() + sizeof(int), packSize);
        m_recvBuffer.remove(0, frameSize);

        DealData(0, payload, packSize);
    }
}

void SslTcpClientMediator::resetSocket()
{
    m_recvBuffer.clear();
    if (!m_socket) {
        return;
    }

    m_socket->disconnect(this);
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
    }
    m_socket->deleteLater();
    m_socket = nullptr;
}
