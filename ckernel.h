#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include "wechatdialog.h"
#include"TcpClientMediator.h"
#include"packdef.h"
#include"logindialog.h"
#include"md5.h"
#include"roomdialog.h"
#include"audioread.h"
#include"audiowrite.h"
#include"videoread.h"
#include"screenread.h"
#include"theradworker.h"
#include <QMutex>

/***************************OPUS优化**************/
#include"AudioWorld.h"
#include"sdlaudioread.h"
#include"sdlaudiowrite.h"
/**********************************************/
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
}
#include"videodecoder.h"
#include"videoencoder.h"

#include "avsyncmanager.h"

//协议映射表的使用类型
class Ckernel;

typedef void (Ckernel::*PFUN) (uint sock,char* buf,int nlen);

class sendVideoWorker;
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
    void SIG_SendVideo(char*buf, int nlen);
public slots:
        //设置协议映射关系
        void setNetPackMap();
        //初始化配置
        void initConfig();
        void slot_destroy();

        //发送登录信息
        void slot_loginCommit(QString tel,QString password);
        //发送注册信息
        void slot_registerCommit(QString tel,QString password,QString name );
        //提交加入房间的申请
        void slot_joinRoom();
        //提交创建房间的申请
        void slot_createRoom();
        //退出房间
        void slot_quitRoom();
        void slot_startAudio();
        void slot_pauseAudio();

        void slot_startVideo();
        void slot_pauseVideo();

        void slot_startScreen();
        void slot_pauseScreen();

        //刷新图片显示
       void slot_refreshVideo(int id,QImage& img);
        //发送音频帧
        void slot_audioFrame(QByteArray ba);

        // 新增：初始化FFmpeg编码器
        //bool initFFmpegEncoder();
        //bool initFFmpegDecoder();

        //发送视频帧
        void slot_sendVideoFrame( QImage img);
        //多线程发送视频
        void slot_SendVideo(char*buf, int nlen);
        //请求验证码
        void slot_sendVefCode(QByteArray  ba);
        //提交验证码
        void slot_commitVefCode(QByteArray ba);

        //网络信息处理
        void slot_dealData(uint sock,char* buf,int nlen);
        //登录回复处理
        void slot_dealLoginRs(uint sock,char* buf,int nlen);
        //注册回复处理
        void slot_dealRegisterRs(uint sock,char* buf,int nlen);
       //创建房间回复
        void slot_dealCreateRoomRs(uint sock,char* buf,int nlen);
        //加入房间的回复
        void slot_dealJoinRoomRs(uint sock,char* buf,int nlen);
        //房间成员请求处理
        void slot_dealRoomMemberRq(uint sock,char* buf,int nlen);
        //离开房间的处理
        void slot_dealLeaveRoomRq(uint sock,char* buf,int nlen);
        //音频帧处理
        void slot_dealAudioFrameRq(uint sock,char* buf,int nlen);
        //视频帧处理
        void slot_dealVideoFrameRq(uint sock,char* buf,int nlen);
        //验证码结果处理
        void slot_dealVefCodeRs(uint sock,char* buf,int nlen);

        void slot_sendEncodedVideo(char *buf, int len);
        void slot_dealVideoH264Rq(uint sock, char *buf, int nlen);
        void slot_showDecodedVideo(int userid, QImage image);
private:
        WeChatDialog* m_pWechatDlg;
        INetMediator* m_pClient;
        LoginDialog* m_pLogindlg;
        RoomDialog* m_pRoomdlg;

        PFUN m_netPackMap[_DEF_PACK_COUNT];

        QString m_serverIp;

        int m_id;
        int m_roomid;
        QString m_name;

        ///////
        /// 音频 一个采集 多个播放 每一个成员 1:1 map映射
        AudioRead* m_pAudioRead;
        std::map<int , AudioWrite*>m_mapIdtoAudioWrite;
        //当前使用SDL音频播放器
        SDLAudioRead* m_pSDLAudioRead;
        std::map<int,SDLAudioWrite*>m_mapIdtoSDLAudioWrite;
        ////////////////////////////
        /// 视频采集
        ///
        VideoRead* m_pVideoRead;
        ScreenRead* m_pScreenRead;

       enum client_type{audio_client = 0,video_client };
         INetMediator* m_pAVClient[2];
        //发送视频的工作线程对应的指针
        QSharedPointer<sendVideoWorker> m_pSendVideoWorker;
        QSharedPointer<sendVideoWorker> m_pSendScreenWorker;
//        // FFmpeg编码相关（保持不变）
//            AVCodec *m_codec = nullptr;
//            AVCodecContext *m_codecCtx = nullptr;
//            AVFrame *m_frame = nullptr;
//            AVPacket *m_packet = nullptr;
//            SwsContext *m_swsCtx = nullptr;

//            // 新增：FFmpeg解码相关
//            AVCodec *m_decoder = nullptr;
//            AVCodecContext *m_decoderCtx = nullptr;
//            AVFrame *m_decFrame = nullptr;    // 解码后的YUV帧
//            AVFrame *m_rgbFrame = nullptr;   // 转换后的RGB帧
//            AVPacket *m_decPacket = nullptr;
//            SwsContext *m_swsDecCtx = nullptr;

//            // 解码缓冲区（用于暂存网络数据）
//            QByteArray m_decBuffer;
//            QMutex m_decMutex;  // 线程安全锁
        /////////////////////////////////////
        /// H.264编解码
        ///
        VideoEncoder* m_videoEncoder;           // 摄像头编码器
        VideoEncoder* m_screenEncoder;          // 桌面编码器
        std::map<int, VideoDecoder*> m_mapIDToDecoder; // 用户ID到解码器的映射

        // 音视频同步管理器
        std::map<int, AVSyncManager*> m_mapIDToSyncManager;



};
class sendVideoWorker : public ThreadWorker
{
    Q_OBJECT
public slots:
    void slot_SendVideo(char* buf,int nlen)
    {
        Ckernel::GetInstance()->slot_sendEncodedVideo(buf,nlen);
    }
};

#endif // CKERNEL_H
