#pragma once

#include "INetMediator.h"

#include <QByteArray>
#include <QSslSocket>

class SslTcpClientMediator : public INetMediator
{
public:
    SslTcpClientMediator();
    ~SslTcpClientMediator();

    bool OpenNet(const char* szBufIP = "0.0.0.0",
                 unsigned short port = _DEF_TCP_PORT);
    void CloseNet();
    bool SendData(unsigned int lSendIP, char* buf, int nlen);
    void DealData(unsigned int lSendIP, char* buf, int nlen);
    void disConnect();
    bool IsConnected();

private:
    void handleReadyRead();
    void resetSocket();

private:
    QSslSocket* m_socket;
    QByteArray m_recvBuffer;
};
