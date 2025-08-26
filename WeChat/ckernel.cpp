#include "ckernel.h"
#include <QDebug>
#include"md5.h"
#include<QMessageBox>
#include<QTime>
#include "BaiduSpeechRecognizer.h"
#include "AudioResampler.h"


#define NetPackMap(a) m_netPackMap[ a - _DEF_PACK_BASE]
//设置协议映射关系
void Ckernel::setNetPackMap()
{
    memset(m_netPackMap,0,sizeof(m_netPackMap));
    NetPackMap(_DEF_PACK_LOGIN_RS)      =&Ckernel::slot_dealLoginRs;
    NetPackMap(_DEF_PACK_REGISTER_RS)   =&Ckernel::slot_dealRegisterRs;
    NetPackMap(_DEF_PACK_CREATEROOM_RS) =&Ckernel::slot_dealCreateRoomRs;
    NetPackMap(_DEF_PACK_JOINROOM_RS)   =&Ckernel::slot_dealJoinRoomRs;
    NetPackMap(_DEF_PACK_ROOM_MEMBER)   =&Ckernel::slot_dealRoomMemberRq;
    NetPackMap(_DEF_PACK_LEAVEROOM_RQ)  =&Ckernel::slot_dealLeaveRoomRq;
    NetPackMap(_DEF_PACK_AUDIO_FRAME)   =&Ckernel::slot_dealAudioFrameRq;
    NetPackMap(_DEF_PACK_VIDEO_FRAME)   =&Ckernel::slot_dealVideoFrameRq;
    NetPackMap(_DEF_PACK_VIDEO_H264) = &Ckernel::slot_dealVideoH264Rq;

}
#include<QSettings>
#include<QApplication>
#include<QFileInfo>
//初始化配置
void Ckernel::initConfig()
{
    m_serverIp=_DEF_SERVERIP;
    //路径设置 exe同级目录下 config.ini
    QString path=QApplication::applicationDirPath()+"/config.ini";
    //判断是否存在
    QFileInfo info(path);
    QSettings settings(path,QSettings::IniFormat,NULL);
    if(info.exists()){
        //加载配置文件 ip 如有配置文件 设置为配置文件中的ip
        //打开配置文件
        //移动到Net组
        settings.beginGroup("Net");
        //读取ip->addr->赋值
        QVariant ip=settings.value("ip");
        QString strIP=ip.toString();
        //结束
        settings.endGroup();
        if(!strIP.isEmpty()){
            m_serverIp=strIP;
        }
    }else{
        //没有配置文件 写入默认的ip
        settings.beginGroup("Net");
        settings.setValue("ip",m_serverIp);
        settings.endGroup();
    }
    qDebug()<<m_serverIp;
}

Ckernel::Ckernel(QObject *parent) : QObject(parent),m_id(0),m_roomid(0),m_isRecognitionEnabled(false)
{
    qDebug()<<"main Thread:"<<QThread::currentThreadId();
    setNetPackMap();
    initConfig();

    m_pWeChatDlg=new WeChatDialog;
    connect(m_pWeChatDlg,SIGNAL(SIG_close())
            ,this,SLOT(slot_destroy()));
    connect(m_pWeChatDlg,SIGNAL(SIG_joinRoom())
            ,this,SLOT(slot_joinRoom()));
    connect(m_pWeChatDlg,SIGNAL(SIG_createRoom())
            ,this,SLOT(slot_createRoom()));
    //m_pWeChatDlg->show();

    m_pLoginDlg=new LoginDialog;
    connect(m_pLoginDlg,SIGNAL(SIG_loginCommit(QString,QString))
            ,this,SLOT(slot_loginCommit(QString,QString)));
    connect(m_pLoginDlg,SIGNAL(SIG_close())
            ,this,SLOT(slot_destroy()));
    connect(m_pLoginDlg,SIGNAL(SIG_registerCommit(QString,QString,QString))
            ,this,SLOT(slot_registerCommit(QString,QString,QString)));
    m_pLoginDlg->show();

    m_pRoomdialog=new RoomDialog;
    connect(m_pRoomdialog,SIGNAL(SIG_close())
            ,this,SLOT(slot_quitRoom()));
    connect(m_pRoomdialog,SIGNAL(SIG_audioPause())
            ,this,SLOT(slot_pauseAudio()));
    connect(m_pRoomdialog,SIGNAL(SIG_audioStart())
            ,this,SLOT(slot_startAudio()));
    connect(m_pRoomdialog,SIGNAL(SIG_videoPause())
            ,this,SLOT(slot_pauseVideo()));
    connect(m_pRoomdialog,SIGNAL(SIG_videoStart())
            ,this,SLOT(slot_startVideo()));
    connect(m_pRoomdialog,SIGNAL(SIG_screenPause())
            ,this,SLOT(slot_pauseScreen()));
    connect(m_pRoomdialog,SIGNAL(SIG_screenStart())
            ,this,SLOT(slot_startScreen()));
    // 字幕控制的信号连接
    connect(m_pRoomdialog,SIGNAL(SIG_startRecognition())
            ,this,SLOT(slot_startAllRecognition()));
    connect(m_pRoomdialog,SIGNAL(SIG_stopRecognition())
            ,this,SLOT(slot_stopAllRecognition()));


    //添加网络
    m_pClient=new TcpClientMediator;

    m_pClient->OpenNet(m_serverIp.toStdString().c_str(),_DEF_PORT);
    connect(m_pClient,SIGNAL(SIG_ReadyData(uint,char*,int))
            ,this,SLOT(slot_dealData(uint,char*,int)));

    //音频和视频的网络连接
    for(int i=0;i<2;i++){
        m_pAVClient[i]=new TcpClientMediator;
        m_pAVClient[i]->OpenNet(m_serverIp.toStdString().c_str(),_DEF_PORT);
        connect(m_pAVClient[i],SIGNAL(SIG_ReadyData(uint,char*,int))
                ,this,SLOT(slot_dealData(uint,char*,int)));
    }

    m_sdlAudioRead=new SDLAudioRead;
    connect(m_sdlAudioRead,SIGNAL(SIG_sendAudioFrame(QByteArray)),
            this,SLOT(slot_audioFrame(QByteArray)));

    m_pVideoRead=new VideoRead;
    connect(m_pVideoRead,SIGNAL(SIG_sendVideoFrame(QImage))
            ,this,SLOT(slot_sendVideoFrame(QImage)));

    m_pScreenRead=new ScreenRead;
    connect(m_pScreenRead,SIGNAL(SIG_getScreenFrame(QImage))
            ,this,SLOT(slot_sendVideoFrame(QImage)));

    m_pSendVideoWorker=QSharedPointer<SendVideoWorker>(new SendVideoWorker);
    connect(this,SIGNAL(SIG_SendVideo(char*,int))
            ,m_pSendVideoWorker.data(),SLOT(slot_sendVideo(char*,int)));

    //设置萌拍效果
    connect(m_pRoomdialog,SIGNAL(SIG_setMoji(int))
            ,m_pVideoRead,SLOT(slot_setMoji(int)));

    // 在构造函数中添加
    // 初始化H.264编码器
    m_videoEncoder = new VideoEncoder;
    m_videoEncoder->init(IMAGE_WIDTH, IMAGE_HEIGHT, FRAME_RATE, 400000);
    connect(m_videoEncoder, SIGNAL(SIG_sendVideoPacket(char*,int)),
            this, SLOT(slot_sendEncodedVideo(char*,int)));
    m_videoEncoder->start();

    // 桌面编码器使用实际分辨率
    QScreen* src = QApplication::primaryScreen();
    QPixmap map = src->grabWindow(QApplication::desktop()->winId());
    m_screenEncoder = new VideoEncoder;
    m_screenEncoder->init(map.width(), map.height(), FRAME_RATE, 1000000);
    connect(m_screenEncoder, SIGNAL(SIG_sendVideoPacket(char*,int)),
            this, SLOT(slot_sendEncodedVideo(char*,int)));
    m_screenEncoder->start();

}



void Ckernel::slot_destroy()
{
    qDebug()<<__func__;
    if(m_pWeChatDlg){
        m_pWeChatDlg->hide();
        delete m_pWeChatDlg;
        m_pWeChatDlg=NULL;
    }
    if(m_pLoginDlg){
        m_pLoginDlg->hide();
        delete m_pLoginDlg;
        m_pLoginDlg=NULL;
    }
    if(m_sdlAudioRead){
        m_sdlAudioRead->slot_closeAudio();
        delete m_sdlAudioRead;
        m_sdlAudioRead=NULL;
    }
    if(m_pRoomdialog){
        m_pRoomdialog->hide();
        delete m_pRoomdialog;
        m_pRoomdialog=NULL;
    }
    if(m_pClient){
        m_pClient->CloseNet();
        delete m_pClient;
        m_pClient=NULL;
    }
    // 清理编码器
    if(m_videoEncoder) {
        delete m_videoEncoder;
        m_videoEncoder = nullptr;
    }
    if(m_screenEncoder) {
        delete m_screenEncoder;
        m_screenEncoder = nullptr;
    }

    exit(0);
}
#define MD5_KEY (1234)
static std::string GetMD5(QString value)
{
    QString str=QString("%1_%2").arg(value).arg(MD5_KEY);
    std::string strSrc=str.toStdString();
    MD5 md5(strSrc);
    return md5.toString();
}
//提交登录信息
void Ckernel::slot_loginCommit(QString tel, QString pass)
{
    std::string strTel=tel.toStdString();
    //std::string strPass=pass.toStdString();

    STRU_LOGIN_RQ rq;
    strcpy(rq.tel,strTel.c_str());

    std::string strPassMD5=GetMD5(pass);
    qDebug()<<strPassMD5.c_str();
    strcpy(rq.password,strPassMD5.c_str());
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//发送注册信息
void Ckernel::slot_registerCommit(QString tel, QString pass, QString name)
{
    std::string strTel=tel.toStdString();
    //std::string strPass=pass.toStdString();
    std::string strName=name.toStdString();
    STRU_REGISTER_RQ rq;
    strcpy(rq.tel,strTel.c_str());

    strcpy(rq.name,strName.c_str());
    std::string strPassMD5=GetMD5(pass);
    qDebug()<<strPassMD5.c_str();
    strcpy(rq.password,strPassMD5.c_str());
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//创建房间
void Ckernel::slot_createRoom()
{
    //判断是否在房间内
    if(m_roomid!=0){
        QMessageBox::about(m_pWeChatDlg,"提示","你已经在一个房间内了");
        return;
    }
    //发命令  创建房间
    STRU_CREATEROOM_RQ rq;
    rq.userid=m_id;

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
#include <QInputDialog>
#include "QRegExp"
//加入房间
void Ckernel::slot_joinRoom()
{
    //判断是否在房间内
    if(m_roomid!=0){
        QMessageBox::about(m_pWeChatDlg,"提示","你已经在一个房间内了");
        return;
    }
    //弹出窗口 填房间号
    QString strRoom=QInputDialog::getText(m_pWeChatDlg,"加入房间","输入房间号");
    //合理化判断
    QRegExp exp("^[0-9]\{1,8\}$");
    if(!exp.exactMatch(strRoom)){
        QMessageBox::about(m_pWeChatDlg,"提示","房间号输入不合法");
        return;
    }
    qDebug()<<strRoom;
    //发命令  加入房间
    STRU_JOINROOM_RQ rq;
    rq.userid=m_id;
    rq.roomid=strRoom.toInt();
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}

//退出房间
void Ckernel::slot_quitRoom()
{
    //发退出包
    STRU_LEAVEROOM_RQ rq;
    rq.userid=m_id;
    rq.roomid=m_roomid;
    std::string name=m_name.toStdString();
    strcpy(rq.name,name.c_str());

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));

    //关闭音频  视频
    m_sdlAudioRead->slot_closeAudio();
    m_pRoomdialog->slot_setAudioCheck(false);
    m_pVideoRead->slot_closeVideo();
    m_pRoomdialog->slot_setVideoCheck(false);
    m_pScreenRead->slot_closeVideo();
    m_pRoomdialog->slot_setScreenCheck(false);

    // 先停止所有同步管理器
    for(auto& pair : m_mapIDToSyncManager) {
        if(pair.second) {
            pair.second->stop();
        }
    }

    // 停止所有解码器
    for(auto& pair : m_mapIDToDecoder) {
        if(pair.second) {
            pair.second->stop();
        }
    }

    // 然后再删除
    for(auto ite=m_mapIDToSyncManager.begin();ite!=m_mapIDToSyncManager.end();){
        AVSyncManager* syncManager = ite->second;
        ite = m_mapIDToSyncManager.erase(ite);
        delete syncManager;
    }

    for(auto ite=m_mapIDToDecoder.begin();ite!=m_mapIDToDecoder.end();){
        VideoDecoder* decoder = ite->second;
        ite = m_mapIDToDecoder.erase(ite);
        delete decoder;
    }

    // 清理其他资源...
    for(auto ite=m_mapIDToSDLAudioWrite.begin();ite!=m_mapIDToSDLAudioWrite.end();){
        SDLAudioWrite* pWrite=ite->second;
        ite=m_mapIDToSDLAudioWrite.erase(ite);
        delete pWrite;
    }

    for(auto ite=m_mapIDToSpeechRecognizer.begin();ite!=m_mapIDToSpeechRecognizer.end();){
        BaiduSpeechRecognizer* pRecognizer=ite->second;
        ite=m_mapIDToSpeechRecognizer.erase(ite);
        delete pRecognizer;
    }

    m_mapIDToUserName.clear();
    m_pRoomdialog->slot_clearUserShow();
    m_roomid=0;
}


void Ckernel::slot_startAudio()
{
    //m_pAudioRead->start();
    m_sdlAudioRead->slot_openAudio();
}

void Ckernel::slot_pauseAudio()
{
    //m_pAudioRead->pause();
    m_sdlAudioRead->slot_closeAudio();
}

void Ckernel::slot_startVideo()
{
    m_pVideoRead->slot_openVideo();
}

void Ckernel::slot_pauseVideo()
{
    m_pVideoRead->slot_closeVideo();
    if(m_roomid != 0) {
        // 发送一个特殊的包告诉其他人你停止了视频
        STRU_VIDEO_H264_V2 stopPack;
        stopPack.type = _DEF_PACK_VIDEO_H264;
        stopPack.userid = m_id;
        stopPack.roomid = m_roomid;
        stopPack.width = 0;  // width=0 表示停止
        stopPack.height = 0;
        stopPack.timestamp = QDateTime::currentMSecsSinceEpoch();
        stopPack.dataLen = 0;

        m_pAVClient[video_client]->SendData(0, (char*)&stopPack, sizeof(stopPack));
    }
}

void Ckernel::slot_startScreen()
{
    m_pScreenRead->slot_openVideo();
}

void Ckernel::slot_pauseScreen()
{
    m_pScreenRead->slot_closeVideo();

    if(m_roomid != 0) {
        STRU_VIDEO_H264_V2 stopPack;
        stopPack.type = _DEF_PACK_VIDEO_H264;
        stopPack.userid = m_id;
        stopPack.roomid = m_roomid;
        stopPack.width = 0;
        stopPack.height = 0;
        stopPack.timestamp = QDateTime::currentMSecsSinceEpoch();
        stopPack.dataLen = 0;

        m_pAVClient[video_client]->SendData(0, (char*)&stopPack, sizeof(stopPack));
    }
}

void Ckernel::slot_refreshVideo(int id,QImage& img)
{
    m_pRoomdialog->slot_refreshUser(id,img);
}


///音频数据帧
///成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min;
/// int sec;
/// int msec;
/// QByteArray audioFrame
//发送音频帧
void Ckernel::slot_audioFrame(QByteArray ba)
{
    // 使用新的数据结构
    int nPackSize = sizeof(STRU_AUDIO_FRAME) + ba.size();
    char* buf = new char[nPackSize];

    STRU_AUDIO_FRAME* pack = (STRU_AUDIO_FRAME*)buf;
    pack->type = _DEF_PACK_AUDIO_FRAME;
    pack->userid = m_id;
    pack->roomid = m_roomid;
    pack->timestamp = QDateTime::currentMSecsSinceEpoch();  // 使用系统时间戳
    pack->dataLen = ba.size();
    memcpy(pack->data, ba.data(), ba.size());

    m_pAVClient[audio_client]->SendData(0, buf, nPackSize);
    delete[] buf;
}
#include<QBuffer>
//发送视频帧
void Ckernel::slot_sendVideoFrame(QImage img)
{
    // 显示图片
    slot_refreshVideo(m_id, img);

    // 判断是摄像头还是桌面
    VideoEncoder* encoder = nullptr;
    if(img.width() == IMAGE_WIDTH && img.height() == IMAGE_HEIGHT) {
        encoder = m_videoEncoder;
    } else {
        encoder = m_screenEncoder;
    }

    // 添加到编码队列
    if(encoder) {
        encoder->addFrame(m_id, m_roomid, img);
    }
}

void Ckernel::slot_sendEncodedVideo(char* buf, int len)
{
    // 更新时间戳为当前系统时间
    STRU_VIDEO_H264_V2* pack = (STRU_VIDEO_H264_V2*)buf;
    pack->timestamp = QDateTime::currentMSecsSinceEpoch();

    m_pAVClient[video_client]->SendData(0, buf, len);
    delete[] buf;
}

void Ckernel::slot_dealVideoH264Rq(uint sock, char* buf, int nlen)
{
    qDebug()<<__func__;
    STRU_VIDEO_H264_V2* pack = (STRU_VIDEO_H264_V2*)buf;

    if(pack->roomid != m_roomid) return;

    if(pack->width == 0 && pack->height == 0) {
        // 这是一个停止信号，清理该用户的解码器
        if(m_mapIDToDecoder.count(pack->userid) > 0) {
            VideoDecoder* decoder = m_mapIDToDecoder[pack->userid];
            m_mapIDToDecoder.erase(pack->userid);
            decoder->stop();
            delete decoder;
            qDebug() << "清理用户" << pack->userid << "的解码器";
        }
        return;
    }

    // 获取或创建解码器
    VideoDecoder* decoder = nullptr;
    if(m_mapIDToDecoder.count(pack->userid) == 0) {
        decoder = new VideoDecoder(pack->userid);
        decoder->init(pack->width, pack->height);

        int userid = pack->userid;

        // 如果有同步管理器，连接到同步管理器
        if(m_mapIDToSyncManager.count(userid) > 0) {
            qDebug() << "为用户" << userid << "连接解码器到同步管理器";

            connect(decoder, &VideoDecoder::SIG_frameDecoded,
                    this, [this, userid](int uid, const QImage& image, qint64 timestamp) {
                qDebug() << "Lambda被调用 - uid:" << uid << "userid:" << userid << "timestamp:" << timestamp;
                if(m_mapIDToSyncManager.count(uid) > 0) {
                    qDebug() << "添加视频帧到同步管理器 - 用户:" << uid;
                    m_mapIDToSyncManager[uid]->addVideoFrame(static_cast<int64_t>(timestamp), image);
                } else {
                    qDebug() << "找不到用户" << uid << "的同步管理器！";
                }
            });
        } else {
            qDebug() << "用户" << userid << "没有同步管理器，直接显示";
            connect(decoder, &VideoDecoder::SIG_frameDecoded,
                    this, [this](int uid, const QImage& image, int64_t timestamp) {
                        m_pRoomdialog->slot_refreshUser(uid, const_cast<QImage&>(image));
                    });
        }

        m_mapIDToDecoder[pack->userid] = decoder;
    } else {
        decoder = m_mapIDToDecoder[pack->userid];

        // 检查分辨率是否改变（这是关键修复）
        if(decoder && (pack->width != decoder->getWidth() || pack->height != decoder->getHeight())) {
            qDebug() << "用户" << pack->userid << "的视频分辨率改变，重新初始化解码器";

            // 停止并删除旧解码器
            decoder->stop();
            delete decoder;
            m_mapIDToDecoder.erase(pack->userid);

            // 创建新解码器
            decoder = new VideoDecoder(pack->userid);
            decoder->init(pack->width, pack->height);

            int userid = pack->userid;
            // 重新连接信号
            if(m_mapIDToSyncManager.count(userid) > 0) {
                connect(decoder, &VideoDecoder::SIG_frameDecoded,
                        this, [this, userid](int uid, const QImage& image, qint64 timestamp) {
                    if(m_mapIDToSyncManager.count(uid) > 0) {
                        m_mapIDToSyncManager[uid]->addVideoFrame(static_cast<int64_t>(timestamp), image);
                    }
                });
            } else {
                connect(decoder, &VideoDecoder::SIG_frameDecoded,
                        this, [this](int uid, const QImage& image, int64_t timestamp) {
                            m_pRoomdialog->slot_refreshUser(uid, const_cast<QImage&>(image));
                        });
            }

            m_mapIDToDecoder[pack->userid] = decoder;
        }
    }

    // 添加数据包到解码队列（传递时间戳）
    if(decoder) {
        decoder->addPacket(pack->data, pack->dataLen, pack->timestamp);
    }
}




void Ckernel::slot_showDecodedVideo(int userid, QImage image)
{
    m_pRoomdialog->slot_refreshUser(userid, image);
}


//多线程发送视频
void Ckernel::slot_SendVideo(char *buf, int nlen)
{
    char* tmp=buf;
    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);

    int min=*(int*)tmp;
    tmp+=sizeof(int);
    int sec=*(int*)tmp;
    tmp+=sizeof(int);
    int msec=*(int*)tmp;
    tmp+=sizeof(int);
    //当前时间
    QTime ctm=QTime::currentTime();
    //数据包时间
    QTime tm(ctm.hour(),min,sec,msec);
    //发送数据包延迟超过300ms 舍弃
    if(tm.msecsTo(ctm)>300){
        qDebug()<<"send fail";
        delete[] buf;
        return;
    }
    //m_pClient->SendData(0,buf,nlen);
    m_pAVClient[video_client]->SendData(0,buf,nlen);
    delete[] buf;
}
//网络处理
void Ckernel::slot_dealData(uint sock, char *buf, int nlen)
{
    //qDebug()<<__func__;
    int type=*(int*)buf;
    if(type>=_DEF_PACK_BASE&&type<_DEF_PACK_BASE+_DEF_PACK_COUNT){
        //取得协议头，根据协议映射关系，找到函数指针
        PFUN pf=NetPackMap(type);
        if(pf){
            (this->*pf)(sock,buf,nlen);
        }
    }
    delete[] buf;
}
//登录回复处理
void Ckernel::slot_dealLoginRs(uint sock, char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_LOGIN_RS* rs=(STRU_LOGIN_RS*)buf;
    switch(rs->result){
    case user_not_exist:
        QMessageBox::about(m_pLoginDlg,"提示","用户不存在，登录失败");
        break;
    case password_error:
        QMessageBox::about(m_pLoginDlg,"提示","密码错误，登录失败");
        break;
    case login_success:
    {
        QString strName =QString("用户[%1]登录成功").arg(rs->name);
        QMessageBox::about(m_pLoginDlg,"提示",strName);
        m_name=QString::fromStdString(rs->name);
        m_pWeChatDlg->setInfo(m_name);
        m_id=rs->userid;
        m_pLoginDlg->hide();
        m_pWeChatDlg->showNormal();

        //注册 视频和音频的fd
        STRU_AUDIO_REGISTER rq_audio;
        rq_audio.userid=m_id;
        STRU_VIDEO_REGISTER rq_video;
        rq_video.userid=m_id;

        m_pAVClient[audio_client]->SendData(0,(char*)&rq_audio,sizeof(rq_audio));
        m_pAVClient[video_client]->SendData(0,(char*)&rq_video,sizeof(rq_video));
    }
        break;
    }
}
//注册回复处理
void Ckernel::slot_dealRegisterRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_REGISTER_RS* rs=(STRU_REGISTER_RS*)buf;
    //根据不同的结果 弹出窗口
    switch(rs->result){
    case tel_is_exist:
        QMessageBox::about(m_pLoginDlg,"提示","手机号已存在，注册失败");
        break;
    case register_success:
        QMessageBox::about(m_pLoginDlg,"提示","注册成功");
        break;
    case name_is_exist:
        QMessageBox::about(m_pLoginDlg,"提示","昵称已存在，注册失败");
        break;
    default: break;
    }
}
//创建房间回复处理
void Ckernel::slot_dealCreateRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_CREATEROOM_RS* rs=(STRU_CREATEROOM_RS*)buf;
    //房间号 显示到界面 跳转

    m_pRoomdialog->slot_setInfo(QString::number(rs->roomid));

    //把自己的信息放到房间里
    UserShow* user=new UserShow;
    connect(user,SIGNAL(SIG_itemClicked(int ,QString))
            ,m_pRoomdialog,SLOT(slot_setBigImgID(int,QString)));
    user->slot_setInfo(m_id,m_name);
    m_pRoomdialog->slot_addUserShow(user);

    m_roomid=rs->roomid;
    m_pRoomdialog->showNormal();

    //初始化音频
    m_pRoomdialog->slot_setAudioCheck(false);

    //视频初始化
    m_pRoomdialog->slot_setVideoCheck(false);
}

//加入房间回复处理
void Ckernel::slot_dealJoinRoomRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_JOINROOM_RS* rs=(STRU_JOINROOM_RS*)buf;
    //根据结果 失败 提示
    if(rs->result==0){
        QMessageBox::about(m_pWeChatDlg,"提示","房间id不存在");
        return;
    }
    qDebug()<<rs->result;
    //成功
    //房间号 显示到界面 跳转
    m_pRoomdialog->slot_setInfo(QString::number(rs->roomid));
    //跳转 roomid 设置
    m_roomid=rs->roomid;
    m_pRoomdialog->showNormal();

    //初始化音频
    m_pRoomdialog->slot_setAudioCheck(false);

    //视频初始化
    m_pRoomdialog->slot_setVideoCheck(false);
}

void Ckernel::slot_dealRoomMemberRq(uint sock, char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_ROOM_MEMBER_RQ* rq=(STRU_ROOM_MEMBER_RQ*)buf;

    // 如果是自己，不需要创建远程音频处理和语音识别
    if(rq->userid == m_id) {
        UserShow* user=new UserShow;
        user->slot_setInfo(rq->userid,QString::fromStdString(rq->name));
        connect(user,SIGNAL(SIG_itemClicked(int ,QString))
                ,m_pRoomdialog,SLOT(slot_setBigImgID(int,QString)));
        m_pRoomdialog->slot_addUserShow(user);
        m_mapIDToUserName[rq->userid] = QString::fromStdString(rq->name);
        return;
    }

    // 处理其他用户
    UserShow* user=new UserShow;
    user->slot_setInfo(rq->userid,QString::fromStdString(rq->name));
    connect(user,SIGNAL(SIG_itemClicked(int ,QString))
            ,m_pRoomdialog,SLOT(slot_setBigImgID(int,QString)));
    m_pRoomdialog->slot_addUserShow(user);
    m_mapIDToUserName[rq->userid] = QString::fromStdString(rq->name);

    // 创建音视频同步管理器
    AVSyncManager* syncManager = new AVSyncManager(rq->userid, this);
    m_mapIDToSyncManager[rq->userid] = syncManager;
    qDebug() << "创建同步管理器成功 - 用户:" << rq->userid;

    // 音频播放
    SDLAudioWrite* aw = new SDLAudioWrite;
    m_mapIDToSDLAudioWrite[rq->userid] = aw;

    // 连接同步管理器的音频播放信号
    connect(syncManager, &AVSyncManager::playAudioFrame,
            aw, &SDLAudioWrite::slot_playAudioFrame);

    // 连接同步管理器的视频播放信号
    int capturedUserid = rq->userid;  // 先保存userid的值
    connect(syncManager, &AVSyncManager::playVideoFrame,
            this, [this, capturedUserid](const QImage& image) {
        m_pRoomdialog->slot_refreshUser(capturedUserid, const_cast<QImage&>(image));
    });


    // 启动同步管理器
    syncManager->start();

    // 为每个远程用户创建独立的语音识别器
    BaiduSpeechRecognizer* recognizer = new BaiduSpeechRecognizer;
    recognizer->init("pahqsVkwngwb5UwuMTm34L4k", "7a2eQBjwK36MBbjXE5JQ4Eij3FmXdsEQ");

    // 只有在字幕功能开启时才启动识别
    if(m_isRecognitionEnabled) {
        recognizer->startRecognition();
    }

    m_mapIDToSpeechRecognizer[rq->userid] = recognizer;

    // 使用更明确的连接方式
    int userid = rq->userid;  // 捕获userid值
    connect(aw, &SDLAudioWrite::SIG_sendDecodedPCM,
            this, [this, userid](const QByteArray& pcmData) {
        this->slot_handleRemoteUserPCM(userid, pcmData);
    });

    // 连接语音识别结果到界面显示
    connect(recognizer, &BaiduSpeechRecognizer::recognitionResult,
            this, [this, userid](const QString& text) {
        this->slot_handleRecognitionResult(userid, text);
    });

    qDebug() << "为用户" << userid << "创建了语音识别器和音视频同步管理器";
}




//离开房间请求处理
void Ckernel::slot_dealLeaveRoomRq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_LEAVEROOM_RQ* rq=(STRU_LEAVEROOM_RQ*)buf;
    //把这个人 从ui上面去掉
    if(rq->roomid==m_roomid){
        m_pRoomdialog->slot_removeUserShow(rq->userid);
    }

    // 清理同步管理器
    if(m_mapIDToSyncManager.count(rq->userid) > 0) {
        AVSyncManager* syncManager = m_mapIDToSyncManager[rq->userid];
        m_mapIDToSyncManager.erase(rq->userid);
        delete syncManager;
    }

    //去掉对应音频
    // 去掉对应音频
    if(m_mapIDToSDLAudioWrite.count(rq->userid)>0){
        SDLAudioWrite* pAw=m_mapIDToSDLAudioWrite[rq->userid];
        m_mapIDToSDLAudioWrite.erase(rq->userid);
        delete pAw;
    }

    // 去掉对应的语音识别器
    if(m_mapIDToSpeechRecognizer.count(rq->userid)>0){
        BaiduSpeechRecognizer* pRecognizer=m_mapIDToSpeechRecognizer[rq->userid];
        m_mapIDToSpeechRecognizer.erase(rq->userid);
        delete pRecognizer;
    }

    // 移除用户名映射
    m_mapIDToUserName.erase(rq->userid);

    // 移除对应的解码器
    if(m_mapIDToDecoder.count(rq->userid) > 0) {
        VideoDecoder* decoder = m_mapIDToDecoder[rq->userid];
        m_mapIDToDecoder.erase(rq->userid);
        delete decoder;
    }

}

//音频帧处理
void Ckernel::slot_dealAudioFrameRq(uint sock, char *buf, int nlen)
{
    STRU_AUDIO_FRAME* pack = (STRU_AUDIO_FRAME*)buf;

    if(m_roomid != pack->roomid) return;

    QByteArray audioData(pack->data, pack->dataLen);

    // 如果有同步管理器，使用同步播放
    if(m_mapIDToSyncManager.count(pack->userid) > 0) {
        AVSyncManager* syncManager = m_mapIDToSyncManager[pack->userid];
        syncManager->addAudioFrame(pack->timestamp, audioData);
    }
    // 否则直接播放（向后兼容）
    else if(m_mapIDToSDLAudioWrite.count(pack->userid) > 0) {
        SDLAudioWrite* aw = m_mapIDToSDLAudioWrite[pack->userid];
        aw->slot_playAudioFrame(audioData);
    }
}

//视频帧处理
void Ckernel::slot_dealVideoFrameRq(uint sock, char *buf, int nlen)
{
    char* tmp=buf;
    tmp+=sizeof(int);
    int userId=*(int*)tmp;
    tmp+=sizeof(int);
    int roomId=*(int*)tmp;
    tmp+=sizeof(int);

    tmp+=sizeof(int);
    tmp+=sizeof(int);
    tmp+=sizeof(int);

    int datalen=nlen-6*sizeof(int);
    QByteArray bt(tmp,datalen);
    QImage img;
    img.loadFromData(bt);
    if(m_roomid==roomId)
        m_pRoomdialog->slot_refreshUser(userId,img);
}


void Ckernel::slot_handleRemoteUserPCM(int userid, const QByteArray& pcmData)
{
    if(!m_isRecognitionEnabled) return;  // 如果识别未开启，直接返回

    qDebug() << "收到用户" << userid << "的PCM数据，大小:" << pcmData.size();

    if(pcmData.isEmpty()) {
        qDebug() << "PCM数据为空!";
        return;
    }

    // 重采样
    QByteArray resampledData = AudioResampler::resample48to16(pcmData);

    qDebug() << "重采样后数据大小:" << resampledData.size();

    // 发送给对应用户的语音识别器
    if(m_mapIDToSpeechRecognizer.count(userid) > 0) {
        m_mapIDToSpeechRecognizer[userid]->addAudioData(resampledData);
    } else {
        qDebug() << "找不到用户" << userid << "的语音识别器!";
    }
}


// 处理语音识别结果
void Ckernel::slot_handleRecognitionResult(int userid, const QString& text)
{
    if(text.isEmpty()) return;

    // 获取用户名
    QString userName = "未知用户";
    if(m_mapIDToUserName.count(userid) > 0) {
        userName = m_mapIDToUserName[userid];
    }

    qDebug() << "用户" << userName << "说:" << text;

    // 发送到界面显示字幕
    m_pRoomdialog->addRemoteSubtitle(userName, text);
}


// 开启所有语音识别
void Ckernel::slot_startAllRecognition()
{
    m_isRecognitionEnabled = true;

    // 启动所有远程用户的语音识别器
    for(auto& pair : m_mapIDToSpeechRecognizer) {
        // 跳过自己的ID
        if(pair.first != m_id && pair.second) {
            pair.second->startRecognition();
            qDebug() << "启动用户" << pair.first << "的语音识别器";
        }
    }

    qDebug() << "已开启所有远程语音识别器";
}

// 关闭所有语音识别
void Ckernel::slot_stopAllRecognition()
{
    m_isRecognitionEnabled = false;

    // 停止所有远程用户的语音识别器
    for(auto& pair : m_mapIDToSpeechRecognizer) {
        // 跳过自己的ID
        if(pair.first != m_id && pair.second) {
            pair.second->stopRecognition();
            qDebug() << "停止用户" << pair.first << "的语音识别器";
        }
    }

    qDebug() << "已关闭所有远程语音识别器";
}
