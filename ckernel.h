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
#include"asrclient.h"
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

//鍗忚鏄犲皠琛ㄧ殑浣跨敤绫诲瀷
class Ckernel;

typedef void (Ckernel::*PFUN) (uint sock,char* buf,int nlen);

class sendVideoWorker;
class Ckernel : public QObject
{
    Q_OBJECT
public:
    explicit Ckernel(QObject *parent = nullptr);

    //鍗曚緥
    static Ckernel* GetInstance()
    {
           static Ckernel kernel;
           return &kernel;
    }
signals:
    void SIG_SendVideo(char*buf, int nlen);
public slots:
        //璁剧疆鍗忚鏄犲皠鍏崇郴
        void setNetPackMap();
        //鍒濆鍖栭厤缃?
        void initConfig();
        void slot_destroy();

        //鍙戦€佺櫥褰曚俊鎭?
        void slot_loginCommit(QString tel,QString password);
        //鍙戦€佹敞鍐屼俊鎭?
        void slot_registerCommit(QString tel,QString password,QString name );
        //鎻愪氦鍔犲叆鎴块棿鐨勭敵璇?
        void slot_joinRoom();
        //鎻愪氦鍒涘缓鎴块棿鐨勭敵璇?
        void slot_createRoom();
        //閫€鍑烘埧闂?
        void slot_quitRoom();
        void slot_startAudio();
        void slot_pauseAudio();

        void slot_startVideo();
        void slot_pauseVideo();

        void slot_startScreen();
        void slot_pauseScreen();

        //鍒锋柊鍥剧墖鏄剧ず
       void slot_refreshVideo(int id,QImage& img);
        //鍙戦€侀煶棰戝抚
        void slot_audioFrame(QByteArray ba, int64_t timestamp = 0);
        // 鏂板锛氬垵濮嬪寲FFmpeg缂栫爜鍣?
        //bool initFFmpegEncoder();
        //bool initFFmpegDecoder();

        //鍙戦€佽棰戝抚
        void slot_sendVideoFrame( QImage img,qint64 time);
        void slot_sendVefCode(QByteArray ba);
        //澶氱嚎绋嬪彂閫佽棰?
        void slot_SendVideo(char*buf, int nlen);
        //璇锋眰楠岃瘉鐮?
        //鎻愪氦楠岃瘉鐮?
        void slot_commitVefCode(QByteArray ba);

        //缃戠粶淇℃伅澶勭悊
        void slot_dealData(uint sock,char* buf,int nlen);
        //鐧诲綍鍥炲澶勭悊
        void slot_dealLoginRs(uint sock,char* buf,int nlen);
        //娉ㄥ唽鍥炲澶勭悊
        void slot_dealRegisterRs(uint sock,char* buf,int nlen);
       //鍒涘缓鎴块棿鍥炲
        void slot_dealCreateRoomRs(uint sock,char* buf,int nlen);
        //鍔犲叆鎴块棿鐨勫洖澶?
        void slot_dealJoinRoomRs(uint sock,char* buf,int nlen);
        //鎴块棿鎴愬憳璇锋眰澶勭悊
        void slot_dealRoomMemberRq(uint sock,char* buf,int nlen);
        //绂诲紑鎴块棿鐨勫鐞?
        void slot_dealLeaveRoomRq(uint sock,char* buf,int nlen);
        //闊抽甯у鐞?
        void slot_dealAudioFrameRq(uint sock,char* buf,int nlen);
        //瑙嗛甯у鐞?
        void slot_dealVideoFrameRq(uint sock,char* buf,int nlen);
        //楠岃瘉鐮佺粨鏋滃鐞?
        void slot_dealVefCodeRs(uint sock,char* buf,int nlen);

        void slot_sendEncodedVideo(char *buf, int len);
        void slot_dealVideoH264Rq(uint sock, char *buf, int nlen);
        void slot_showDecodedVideo(int userid, QImage image);
        void slot_recognitionChanged(bool enabled);
        void slot_asrTextRecognized(int userId, const QString& text, bool isFinal, qint64 timestamp);
        void slot_asrStatusChanged(bool available, const QString& message);
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
        /// 闊抽 涓€涓噰闆?澶氫釜鎾斁 姣忎竴涓垚鍛?1:1 map鏄犲皠
        AudioRead* m_pAudioRead;
        std::map<int , AudioWrite*>m_mapIdtoAudioWrite;
        //褰撳墠浣跨敤SDL闊抽鎾斁鍣?
        SDLAudioRead* m_pSDLAudioRead;
        std::map<int,SDLAudioWrite*>m_mapIdtoSDLAudioWrite;
        std::map<int, QString> m_mapIdToName;
        ////////////////////////////
        /// 视频采集
        VideoRead* m_pVideoRead;
        ScreenRead* m_pScreenRead;

       enum client_type{audio_client = 0,video_client };
         INetMediator* m_pAVClient[2];
        //鍙戦€佽棰戠殑宸ヤ綔绾跨▼瀵瑰簲鐨勬寚閽?
        QSharedPointer<sendVideoWorker> m_pSendVideoWorker;
        QSharedPointer<sendVideoWorker> m_pSendScreenWorker;
//        // FFmpeg缂栫爜鐩稿叧锛堜繚鎸佷笉鍙橈級
//            AVCodec *m_codec = nullptr;
//            AVCodecContext *m_codecCtx = nullptr;
//            AVFrame *m_frame = nullptr;
//            AVPacket *m_packet = nullptr;
//            SwsContext *m_swsCtx = nullptr;

//            // 鏂板锛欶Fmpeg瑙ｇ爜鐩稿叧
//            AVCodec *m_decoder = nullptr;
//            AVCodecContext *m_decoderCtx = nullptr;
//            AVFrame *m_decFrame = nullptr;    // 瑙ｇ爜鍚庣殑YUV甯?
//            AVFrame *m_rgbFrame = nullptr;   // 杞崲鍚庣殑RGB甯?
//            AVPacket *m_decPacket = nullptr;
//            SwsContext *m_swsDecCtx = nullptr;

//            // 瑙ｇ爜缂撳啿鍖猴紙鐢ㄤ簬鏆傚瓨缃戠粶鏁版嵁锛?
//            QByteArray m_decBuffer;
//            QMutex m_decMutex;  // 绾跨▼瀹夊叏閿?
        /////////////////////////////////////
        /// H.264缂栬В鐮?
        ///
        VideoEncoder* m_videoEncoder;           // 鎽勫儚澶寸紪鐮佸櫒
        VideoEncoder* m_screenEncoder;          // 妗岄潰缂栫爜鍣?
        std::map<int, VideoDecoder*> m_mapIDToDecoder; // 鐢ㄦ埛ID鍒拌В鐮佸櫒鐨勬槧灏?

        // 闊宠棰戝悓姝ョ鐞嗗櫒
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
