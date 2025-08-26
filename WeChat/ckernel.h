#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include <QDialog>
#include"wechatdialog.h"
#include "TcpClientMediator.h"
#include"packdef.h"
#include "logindialog.h"
#include "roomdialog.h"

#include"sdlaudioread.h"
#include"sdlaudiowrite.h"

#include"videoread.h"

#include"screenread.h"
#include"threadworker.h"

#include"videodecoder.h"
#include"videoencoder.h"

#include "avsyncmanager.h"

//协议映射表使用的类型
class Ckernel;
typedef void(Ckernel::*PFUN)(uint sock,char* buf,int nlen);

class SendVideoWorker;
class Ckernel : public QObject
{
    Q_OBJECT
public:
    explicit Ckernel(QObject *parent = nullptr);

    //单例
    static Ckernel* GetInstance()
    {
        static Ckernel kernel;
        return &kernel;
    }
signals:
    void SIG_SendVideo(char* buf,int nlen);
public slots:
    //初始化协议映射关系
    void setNetPackMap();
    //初始化配置
    void initConfig();
    //回收
    void slot_destroy();
    //发送登录信息
    void slot_loginCommit(QString tel,QString pass);
    //发送注册信息
    void slot_registerCommit(QString tel,QString pass,QString name);

    void slot_createRoom();
    void slot_joinRoom();
    void slot_quitRoom();

    void slot_startAudio();
    void slot_pauseAudio();

    void slot_startVideo();
    void slot_pauseVideo();

    void slot_startScreen();
    void slot_pauseScreen();
    //刷新图片显示
    void slot_refreshVideo(int id, QImage &img);
    //发送音频帧
    void slot_audioFrame(QByteArray ba);
    //void slot_sendAudioFrame(QByteArray ba);
    //发送视频帧
    void slot_sendVideoFrame(QImage img);
    //多线程发送视频
    void slot_SendVideo(char* buf,int nlen);
    ///网络信息处理
    void slot_dealData(uint sock,char* buf,int nlen);
    //登录回复处理
    void slot_dealLoginRs(uint sock,char* buf,int nlen);
    //注册回复处理
    void slot_dealRegisterRs(uint sock,char* buf,int nlen);
    //创建房间回复处理
    void slot_dealCreateRoomRs(uint sock,char* buf,int nlen);
    //加入房间回复处理
    void slot_dealJoinRoomRs(uint sock,char* buf,int nlen);
    //房间成员请求处理
    void slot_dealRoomMemberRq(uint sock,char* buf,int nlen);
    //离开房间请求处理
    void slot_dealLeaveRoomRq(uint sock,char* buf,int nlen);
    //音频帧处理
    void slot_dealAudioFrameRq(uint sock,char* buf,int nlen);
    //视频帧处理
    void slot_dealVideoFrameRq(uint sock,char* buf,int nlen);

    // 语音识别相关
    void slot_handleRemoteUserPCM(int userid, const QByteArray& pcmData);
    void slot_handleRecognitionResult(int userid, const QString& text);
    // 语音识别控制
    void slot_startAllRecognition();
    void slot_stopAllRecognition();

    // H.264视频帧处理
    void slot_dealVideoH264Rq(uint sock, char* buf, int nlen);
    // 编码后视频发送
    void slot_sendEncodedVideo(char* buf, int len);
    // 解码后视频显示
    void slot_showDecodedVideo(int userid, QImage image);
private:
    WeChatDialog* m_pWeChatDlg;
    LoginDialog* m_pLoginDlg;
    INetMediator* m_pClient;
    QString m_serverIp;
    RoomDialog* m_pRoomdialog;
    //协议映射表
    PFUN m_netPackMap[_DEF_PACK_COUNT];


    QString m_name;
    int m_id;
    int m_roomid;

    /////////////////////////////////////
    /// 音频 1个采集 多个播放 每一个房间成员  1:1 map映射
    ///
//    AudioRead* m_pAudioRead;
//    std::map<int,AudioWrite*> m_mapIDToAudioWrite;

    SDLAudioRead* m_sdlAudioRead;
    std::map<int,SDLAudioWrite*> m_mapIDToSDLAudioWrite;

    /////////////////////////////////////
    /// 视频
    ///
    VideoRead* m_pVideoRead;
    ScreenRead* m_pScreenRead;

    enum client_type{audio_client=0,video_client};
    INetMediator* m_pAVClient[2];

    QSharedPointer<SendVideoWorker> m_pSendVideoWorker;

    /////////////////////////////////////
    /// 语音识别相关
    ///
    std::map<int, BaiduSpeechRecognizer*> m_mapIDToSpeechRecognizer;  // 每个用户的语音识别器
    std::map<int, QString> m_mapIDToUserName;  // 用户ID到名字的映射

    bool m_isRecognitionEnabled;  // 字幕功能是否开启

    /////////////////////////////////////
    /// H.264编解码
    ///
    VideoEncoder* m_videoEncoder;           // 摄像头编码器
    VideoEncoder* m_screenEncoder;          // 桌面编码器
    std::map<int, VideoDecoder*> m_mapIDToDecoder; // 用户ID到解码器的映射

    // 音视频同步管理器
    std::map<int, AVSyncManager*> m_mapIDToSyncManager;
};

class SendVideoWorker:public ThreadWorker
{
    Q_OBJECT
public slots:
    void slot_sendVideo(char* buf,int nlen){
        Ckernel::GetInstance()->slot_SendVideo(buf,nlen);
    }
};

#endif // CKERNEL_H
