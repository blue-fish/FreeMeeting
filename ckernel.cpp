#include "ckernel.h"
#include"QDebug"
#include"QSettings"
#include"QApplication"
#include"QFileInfo"
#include"QMessageBox"
#include"QInputDialog"
#include"QRegExp"
#include"QTime"
#define NetPackMap(a) m_netPackMap[a - _DEF_PACK_BASE]

//设置协议映射关系
void Ckernel::setNetPackMap()
{
    memset(m_netPackMap,0,sizeof (m_netPackMap));
    NetPackMap(_DEF_PACK_REGISTER_RS) = &Ckernel::slot_dealRegisterRs;
    NetPackMap(_DEF_PACK_LOGIN_RS) = &Ckernel::slot_dealLoginRs;
    NetPackMap(DEF_PACK_CREATEROOM_RS) = &Ckernel::slot_dealCreateRoomRs;
    NetPackMap(DEF_PACK_JOINROOM_RS) = &Ckernel::slot_dealJoinRoomRs;
    NetPackMap(DEF_PACK_ROOM_MEMBER) = &Ckernel::slot_dealRoomMemberRq;
    NetPackMap(DEF_PACK_LEAVEROOM_RQ) = &Ckernel::slot_dealLeaveRoomRq;
    NetPackMap(DEF_PACK_AUDIO_FRAME) = &Ckernel::slot_dealAudioFrameRq;
    NetPackMap(DEF_PACK_VIDEO_FRAME) = &Ckernel::slot_dealVideoFrameRq;
    NetPackMap(DEF_PACK_VEFCODE_RES) = &Ckernel::slot_dealVefCodeRs;
    NetPackMap(_DEF_PACK_VIDEO_H264) = &Ckernel::slot_dealVideoH264Rq;
 }
//初始化配置
void Ckernel::initConfig()
{

    m_serverIp = _DEF_SERVERIP;

    // 路径为 exe同级的目录 config.ini
    QString path=QApplication::applicationDirPath()+"config.ini";
    //判断路径是否存在
    QFileInfo info(path);
    //打开配置文件
    QSettings settings(path,QSettings::IniFormat,NULL); //有就打开，没有创建
    if(info.exists())
    {
        //加载配置文件 ip设置为配置文件中的ip

            //移动到IP组
            settings.beginGroup("Net");
            //读取，赋值
            QVariant ip= settings.value("ip");
            QString strIp=ip.toString();
            //结束
            settings.endGroup();
            if(!strIp.isEmpty())
            {
                m_serverIp = strIp;

            }
    }else
    {
         //没有配置文件  写入默认的IP
          settings.beginGroup("Net");
          settings.setValue("ip",m_serverIp);
          settings.endGroup();
    }
      qDebug()<<m_serverIp;

}

Ckernel::Ckernel(QObject *parent) : QObject(parent)
{
     qDebug()<<"main thread:"<<QThread::currentThread();
     setNetPackMap();
     initConfig();
    m_pWechatDlg = new WeChatDialog;
    connect(m_pWechatDlg,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    connect(m_pWechatDlg,SIGNAL(SIG_createRoom()),this,SLOT(slot_createRoom()));
    connect(m_pWechatDlg,SIGNAL(SIG_joinRoom()),this,SLOT(slot_joinRoom()));
    //m_pWechatDlg->show();
    m_pLogindlg = new LoginDialog;

    connect(m_pLogindlg,SIGNAL(SIG_loginCommit(QString ,QString)),this,SLOT(slot_loginCommit(QString ,QString)));
    connect(m_pLogindlg,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    connect(m_pLogindlg,SIGNAL(SIG_registerCommit(QString,QString,QString )),this,SLOT(slot_registerCommit(QString,QString,QString )));
    connect(m_pLogindlg,SIGNAL(SIG_sendVefCode(QByteArray  )),this,SLOT(slot_sendVefCode(QByteArray  )));
    connect(m_pLogindlg,SIGNAL(SIG_commitVefCode(QByteArray  )),this,SLOT(slot_commitVefCode(QByteArray  )));

    m_pLogindlg->show();

    m_pRoomdlg = new RoomDialog;
    connect(m_pRoomdlg,SIGNAL(SIG_close()),this,SLOT(slot_quitRoom()));
    //connect(m_pRoomdlg,SIGNAL(SIG_audioPause()),this,SLOT(slot_pauseAudio()));
    //connect(m_pRoomdlg,SIGNAL(SIG_audioStart()),this,SLOT(slot_startAudio()));
    //sdl 音频开启结束
    connect(m_pRoomdlg,SIGNAL(SIG_SDLaudioPause()),this,SLOT(slot_pauseAudio()));
    connect(m_pRoomdlg,SIGNAL(SIG_SDLaudioStart()),this,SLOT(slot_startAudio()));

    connect(m_pRoomdlg,SIGNAL(SIG_videoPause()),this,SLOT(slot_pauseVideo()));
    connect(m_pRoomdlg,SIGNAL(SIG_videoStart()),this,SLOT(slot_startVideo()));
    connect(m_pRoomdlg,SIGNAL(SIG_screenPause()),this,SLOT(slot_pauseScreen()));
    connect(m_pRoomdlg,SIGNAL(SIG_screenStart()),this,SLOT(slot_startScreen()));
    //添加网络--第一条tcp连接
    m_pClient=new TcpClientMediator;
   // m_pClient->OpenNet( m_serverIp.toStdString().c_str(),_DEF_TCP_PORT );
    m_pClient->OpenNet( _DEF_SERVERIP,_DEF_TCP_PORT );
    connect(m_pClient,SIGNAL(SIG_ReadyData(uint,char*,int)),this,SLOT(slot_dealData(uint,char*,int)));

    //音频和视频的连接---第二条和第三条tcp连接
    for(int i=0;i<2;i++)
    {
        m_pAVClient[i] = new TcpClientMediator;
        m_pAVClient[i]->OpenNet( _DEF_SERVERIP,_DEF_TCP_PORT );
        connect(m_pAVClient[i],SIGNAL(SIG_ReadyData(uint,char*,int)),
                this,SLOT(slot_dealData(uint,char*,int)));
    }


#ifdef USE_OPUS
    //SDL的音频采集
    m_pSDLAudioRead = new SDLAudioRead;
    connect(m_pSDLAudioRead,SIGNAL(SIG_sendAudioFrame(QByteArray ))
            ,this,SLOT(slot_audioFrame(QByteArray)));

#else
    //QT 的音频采集
    m_pAudioRead = new AudioRead;
    connect(m_pAudioRead,SIGNAL(SIG_audioFrame(QByteArray)),
            this,SLOT(slot_audioFrame(QByteArray)));
#endif


    m_pVideoRead = new VideoRead;
    connect(m_pVideoRead,SIGNAL(SIG_sendVideoFrame(QImage)),
            this,SLOT(slot_sendVideoFrame(QImage)));

    m_pScreenRead = new ScreenRead;
    connect(m_pScreenRead,SIGNAL( SIG_getScreenFrame(QImage)),
            this,SLOT(slot_sendVideoFrame(QImage)));

    m_pSendVideoWorker = QSharedPointer<sendVideoWorker> (new sendVideoWorker);
    connect(this ,SIGNAL(SIG_SendVideo(char*,int)),
            m_pSendVideoWorker.data() ,SLOT(slot_SendVideo(char*,int)));

    //设置萌拍效果
    connect(m_pRoomdlg,SIGNAL(SIG_setMoji(int)),m_pVideoRead,SLOT(slot_setMoji(int)));

    // 在构造函数中添加
    // 初始化H.264编码器
     m_pSendVideoWorker = QSharedPointer<sendVideoWorker> (new sendVideoWorker);

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
    if(m_pWechatDlg)
    {
        delete m_pWechatDlg;m_pWechatDlg=NULL;
    }
    if(m_pLogindlg)
    {
        m_pLogindlg->hide();
        delete  m_pLogindlg;
        m_pLogindlg=NULL;
    }
    if(m_pAudioRead)
    {
        m_pAudioRead->pause();
        delete m_pAudioRead;
        m_pAudioRead=NULL;
    }
    if(m_pSDLAudioRead)
    {
        m_pSDLAudioRead->slot_closeAudio();
        delete m_pSDLAudioRead;
        m_pSDLAudioRead=NULL;
    }
    if(m_pRoomdlg)
    {
        m_pRoomdlg->hide();
        delete  m_pRoomdlg;
        m_pRoomdlg=NULL;
    }
    if(m_pClient)
    {
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
    QString str = QString ("%1_%2").arg(value).arg(MD5_KEY);
    std::string strSrc = str.toStdString();
    MD5 md5(strSrc);
    return  md5.toString();
}

//提交登录
void Ckernel::slot_loginCommit(QString tel, QString password)
{
    qDebug()<<__func__;
    std::string strTel =tel.toStdString();

    STRU_LOGIN_RQ rq;
    strcpy(rq.tel,strTel.c_str());
    std::string strPassMD5 = GetMD5 (password);
    strcpy(rq.password,strPassMD5.c_str());

    qDebug()<<strPassMD5.c_str();

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//发送注册信息
void Ckernel::slot_registerCommit(QString tel, QString password, QString name)
{
    qDebug()<<__func__;
    std::string strTel =tel.toStdString();
    std::string strName = name.toStdString();  //格式UTF8

    STRU_REGISTER_RQ rq;
    strcpy(rq.tel,strTel.c_str());

    std::string strPassMD5 = GetMD5 (password);
    strcpy(rq.password,strPassMD5.c_str());
    strcpy(rq.name,strName.c_str());

    qDebug()<<strPassMD5.c_str();
    //兼容中文 utf8 QString->std::string  --> char*
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}

//数据处理
void Ckernel::slot_dealData(uint sock, char *buf, int nlen)
{
     qDebug()<<__func__;
    int type= *(int*)buf;
    if(type>=_DEF_PACK_BASE && type<= _DEF_PACK_BASE + _DEF_PACK_COUNT)
    {
        PFUN pf = NetPackMap(type);
        if(pf)
        {
            (this->*pf)(sock,buf,nlen);
        }
    }
    delete []buf;
}
//登录回复处理
void Ckernel::slot_dealLoginRs(uint sock, char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_LOGIN_RS* rs =(STRU_LOGIN_RS*)buf;
    switch (rs->result) {
    case tel_not_exist:
        QMessageBox::about(m_pLogindlg,"提示","电话号码不存在");
        break;

    case password_error:
        QMessageBox::about(m_pLogindlg,"提示","密码错误");
        break;
    case login_success:
    {
        QString strName = QString("[%1]登录成功").arg(rs->m_name);
        QMessageBox::about(m_pLogindlg,"提示",strName);
        //id 和名字记录
        m_id = rs->userid;
        m_name=QString::fromStdString( rs->m_name);
        m_pLogindlg->hide();
        m_pWechatDlg->setInfo(rs->m_name);
        m_pWechatDlg->showNormal();
        //注册音频和视频的fd
        STRU_AUDIO_REGISTER rq_audio;
        rq_audio.m_userid = m_id;
        STRU_VIDEO_REGISTER rq_video;
        rq_video.m_userid = m_id;
        m_pAVClient[audio_client]->SendData(0,(char*)&rq_audio,sizeof (rq_audio));
        m_pAVClient[video_client]->SendData(0,(char*)&rq_video,sizeof (rq_video));
        break;
    }
    default:
        break;
    }
}
//注册回复处理
void Ckernel::slot_dealRegisterRs(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_REGISTER_RS* rs =(STRU_REGISTER_RS*)buf;
    switch (rs->result) {
    case tel_is_exist:
        QMessageBox::about(m_pLogindlg,"提示","电话号码已存在");
        break;
    case name_is_exist:
        QMessageBox::about(m_pLogindlg,"提示","昵称已存在");
        break;
    case register_success:
    {
        QMessageBox::about(m_pLogindlg,"提示","注册成功");

        break;
    }
    default:
        break;
    }
}
//处理创建房间回复
void Ckernel::slot_dealCreateRoomRs(uint sock, char *buf, int nlen)
{
     qDebug()<<__func__;
    //拆包
    STRU_CREATEROOM_RS* rs=(STRU_CREATEROOM_RS*)buf;
    //房间号 显示到界面 跳转
    m_pRoomdlg->slot_setInfo(QString::number( rs->m_RoomId));
    //自己是房间的第一个用户，服务器没有把信息发给自己，todo :需要自己添加个人信息到房间里
    UserShow* user = new UserShow;

    connect(user,SIGNAL(SIG_itemClicked(int,QString)),
            m_pRoomdlg,SLOT(slot_setBigImageId(int,QString)));

    user->slot_setInfo(m_id,m_name);
    m_pRoomdlg->slot_addUserShow(user);

    m_roomid = rs->m_RoomId;

    m_pRoomdlg->showNormal();
    //音频初始化
    m_pRoomdlg->slot_setAudioCheck(false);
    //视频初始化
    m_pRoomdlg->slot_setVideoCheck(false);
}
//处理加入房间回复
void Ckernel::slot_dealJoinRoomRs(uint sock, char *buf, int nlen)
{
     qDebug()<<__func__ ;
     //拆包
     STRU_JOINROOM_RS* rs=(STRU_JOINROOM_RS*)buf;
     //结果失败提示
     if(rs->m_lResult==0)
     {
         QMessageBox::about(m_pWechatDlg,"提示","房间id不存在，加入失败");
         return;
     }
     //成功 跳转 roomid设置
     qDebug()<<rs->m_RoomID;
      m_pRoomdlg->slot_setInfo(QString::number(rs->m_RoomID));
     m_roomid = rs->m_RoomID;
     m_pRoomdlg->showNormal();

     //音频初始化
     m_pRoomdlg->slot_setAudioCheck(false);
     //视频初始化
     m_pRoomdlg->slot_setVideoCheck(false);
}
// 房间成员请求处理
void Ckernel::slot_dealRoomMemberRq(uint sock, char *buf, int nlen)
{
      qDebug()<<__func__ ;
    //拆包
    STRU_ROOM_MEMBER_RQ* rq = (STRU_ROOM_MEMBER_RQ*)buf;

    qDebug()<<__func__<<"房间成员姓名："<<rq->m_szUser;
    //创建用户对应的控件
    UserShow* user = new UserShow;
    user->slot_setInfo(rq->m_UserID,QString::fromStdString(rq->m_szUser));
    connect(user,SIGNAL(SIG_itemClicked(int,QString)),
            m_pRoomdlg,SLOT(slot_setBigImageId(int,QString)));
    m_pRoomdlg->slot_addUserShow(user);

   #ifdef USE_OPUS
    //音频内容
    SDLAudioWrite* aw =  NULL;
    //为每个人创建播放对象
    if(m_mapIdtoAudioWrite.count(rq->m_UserID)==0)
    {
        aw = new SDLAudioWrite;
        m_mapIdtoSDLAudioWrite[rq->m_UserID]=aw ;
    }
   #else
    //音频内容
    AudioWrite* aw =  NULL;
    //为每个人创建播放对象
    if(m_mapIdtoAudioWrite.count(rq->m_UserID)==0)
    {
        aw = new AudioWrite;
        m_mapIdtoAudioWrite[rq->m_UserID]=aw ;
    }
#endif



}
//离开房间处理（把离开者的信息从ui界面上面去除）
void Ckernel::slot_dealLeaveRoomRq(uint sock, char *buf, int nlen)
{
    //拆包
    STRU_LEAVEROOM_RQ*rq = (STRU_LEAVEROOM_RQ*)buf;

    if(rq->m_RoomId == m_roomid)
    {
        m_pRoomdlg->slot_removeUserShow(rq->m_nUserId);
    }
    //去掉对应音频
    if(m_mapIdtoAudioWrite.count(rq->m_nUserId)>0)
    {
        AudioWrite * pAw =m_mapIdtoAudioWrite[rq->m_nUserId];
        m_mapIdtoAudioWrite.erase(rq->m_nUserId);
        delete pAw;
    }


}
//音频帧处理
void Ckernel::slot_dealAudioFrameRq(uint sock, char *buf, int nlen)
{

    ///音频数据帧
    /// 成员描述
    /// int type;
    /// int userId;
    /// int roomId;
    /// int min;
    /// int sec;
    /// int msec;
    /// QByteArray audioFrame;
    //序列化

    int userId=m_id;
    int roomId = m_roomid;
    char* tmp=buf;

    tmp += sizeof(int);
    //按照整形取
    userId=*(int*)tmp ;
    tmp += sizeof(int);

    roomId=*(int*)tmp ;
    tmp += sizeof(tmp);


    tmp += sizeof(int);

    tmp += sizeof(int);

    tmp += sizeof(int);

    int nbufLen=nlen-sizeof(int)*6;
    QByteArray ba (tmp,nbufLen);

    if(m_roomid == roomId)
    {
        #ifdef USE_OPUS
                if(m_mapIdtoSDLAudioWrite.count(userId)>0 )
                {
                    SDLAudioWrite* aw = m_mapIdtoSDLAudioWrite[userId];
                    aw->slot_playAudioFrame(ba);
                }
        #else
                if(m_mapIdtoAudioWrite.count(userId)>0 )
                {
                    AudioWrite* aw = m_mapIdtoAudioWrite[userId];
                    aw->slot_playAudio(ba);
                }
        #endif
    }
}

void Ckernel::slot_dealVideoFrameRq(uint sock, char *buf, int nlen)
{
//    //拆包
//    ///视频数据帧
//    /// 成员描述
//    /// int type;
//    /// int userId;
//    /// int roomId;
//    /// int min;
//    /// int sec;
//    /// int msec;
//    /// QByteArray videoFrame;

////
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
                        m_pRoomdlg->slot_refreshUser(uid, const_cast<QImage&>(image));
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
                            m_pRoomdlg->slot_refreshUser(uid, const_cast<QImage&>(image));
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
    m_pRoomdlg->slot_refreshUser(userid, image);
}
void Ckernel::slot_dealVefCodeRs(uint sock, char *buf, int nlen)
{
    qDebug()<<__func__;
    STRU_VEFCODE_RS* rs=(STRU_VEFCODE_RS*)buf;
    if(rs->m_res==login_success)
    {
        QString strName = QString("[%1]登录成功").arg(rs->m_name);
        QMessageBox::about(m_pLogindlg,"提示",strName);
        //id 和名字记录
        m_id = rs->m_userid;
        m_name=QString::fromStdString( rs->m_name);
        m_pLogindlg->hide();
        m_pWechatDlg->setInfo(rs->m_name);
        m_pWechatDlg->showNormal();
        //注册音频和视频的fd
        STRU_AUDIO_REGISTER rq_audio;
        rq_audio.m_userid = m_id;
        STRU_VIDEO_REGISTER rq_video;
        rq_video.m_userid = m_id;
        m_pAVClient[audio_client]->SendData(0,(char*)&rq_audio,sizeof (rq_audio));
        m_pAVClient[video_client]->SendData(0,(char*)&rq_video,sizeof (rq_video));

    }
    else if(rs->m_res==vefcode_error){
        QMessageBox::about(m_pLogindlg,"提示","验证码错误");
    }
    else if(rs->m_res==tel_error){
        QMessageBox::about(m_pLogindlg,"提示","请使用申请验证码的手机号进行验证");
    }
    else
    {
        qDebug()<<"slot_dealVefCodeRs :rs->m_res="<<rs->m_res;
         QMessageBox::about(m_pLogindlg,"提示","该手机号未注册，请先注册");
    }
}
//加入房间
void Ckernel::slot_joinRoom()
{

   qDebug()<<__func__ ;
    //判断是否在房间内
    if(m_roomid!=0)
    {
        QMessageBox::about(m_pWechatDlg,"提示","在房间内，无法加入，先退出");
        return;
    }

    //弹出窗口 添房间号
    QString strRoom = QInputDialog::getText(m_pWechatDlg,"加入房间","请输入房间号");
    QRegExp exp("^[0-9]\{1,8\}$");

    if( !exp.exactMatch(strRoom))
    {
        QMessageBox::about(m_pWechatDlg,"提示","房间号不合法");
        return;
    }
    //发命令 加入房间

    STRU_JOINROOM_RQ rq;
    rq.m_UserID = m_id;
    rq.m_RoomID = strRoom.toInt();
    m_pClient->SendData(0,(char*)&rq, sizeof(rq));
}
//创建房间
void Ckernel::slot_createRoom()
{
    //判断是否在房间内 m_roomid
     qDebug()<<__func__ ;
    if(m_roomid!=0)
    {
        QMessageBox::about(m_pWechatDlg,"提示","在房间内，无法创建，先退出");
        return;
    }

    //发命令 创建房间
    STRU_CREATEROOM_RQ rq;
    rq.m_UserID = m_id;

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}
//退出房间
void Ckernel::slot_quitRoom()
{
    qDebug()<<__func__ ;

    //发退出包
    STRU_LEAVEROOM_RQ rq;
    rq.m_nUserId=m_id;
    rq.m_RoomId = m_roomid;
    std::string name = m_name.toStdString();
    strcpy(rq.szUserName,name.c_str());

    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
    //关闭 音频 视频(音频放到下面关)
//    m_pAudioRead->pause();
//    m_pSDLAudioRead->slot_closeAudio();
    m_pVideoRead->slot_closeVideo();
    m_pScreenRead->slot_closeVideo();


    m_pRoomdlg->slot_setAudioCheck(false);
    m_pRoomdlg->slot_setVideoCheck(false);
    m_pRoomdlg->slot_setScreenCheck(false);
    //回收所有人的audio_writre
   #ifdef USE_OPUS
    m_pSDLAudioRead->slot_closeAudio();
    for(auto ite =  m_mapIdtoSDLAudioWrite.begin();ite!=m_mapIdtoSDLAudioWrite.end();)
    {
        SDLAudioWrite * pWrite = ite->second;
        ite = m_mapIdtoSDLAudioWrite.erase(ite);
        delete pWrite;
    }

#else
    m_pAudioRead->pause();
    for(auto ite = m_mapIdtoAudioWrite.begin();ite!=m_mapIdtoAudioWrite.end();)
    {
        AudioWrite * pWrite = ite->second;
        ite = m_mapIdtoAudioWrite.erase(ite);
        delete pWrite;
    }
#endif
    m_roomid=0;
    //回收资源
    m_pRoomdlg->slot_clearUserShow();
}
//开启音频
void Ckernel::slot_startAudio()
{
#ifdef USE_OPUS
    m_pSDLAudioRead->slot_openAudio();
#else
    m_pAudioRead->start();
#endif
}
//关闭音频
void Ckernel::slot_pauseAudio()
{
#ifdef USE_OPUS
    m_pSDLAudioRead->slot_closeAudio();
#else
    m_pAudioRead->pause();
#endif

}

//开启视频
void Ckernel::slot_startVideo()
{
    qDebug()<<__func__ ;
    m_pVideoRead->slot_openVideo();
}
//关闭视频
void Ckernel::slot_pauseVideo()
{
    m_pVideoRead->slot_closeVideo();
}

//开启桌面
void Ckernel::slot_startScreen()
{
    qDebug()<<__func__ ;
    m_pScreenRead->slot_openVideo();
}
//关闭桌面
void Ckernel::slot_pauseScreen()
{
    m_pScreenRead->slot_closeVideo();
}

void Ckernel::slot_refreshVideo(int id, QImage &img)
{
    m_pRoomdlg->slot_refreshUser(id,img);
}
//发送音频帧
///音频数据帧
/// 成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min;
/// int sec;
/// int msec;
/// QByteArray audioFrame;
void Ckernel::slot_audioFrame(QByteArray ba)
{
    int nPackSize = 6*sizeof(int)+ba.size();
    char* buf = new char[nPackSize];
    char* tmp =buf;
    //序列化
    int type = DEF_PACK_AUDIO_FRAME;

    int userId=m_id;
    int roomId = m_roomid;
    QTime tm = QTime::currentTime();
    int min = tm.minute();
    int sec = tm.second();
    int msec = tm.msec();

    *(int*)tmp = type;
    tmp += sizeof(tmp);
    //按照整形存
    *(int*)tmp = userId;
    tmp += sizeof(tmp);

    *(int*)tmp = roomId;
    tmp += sizeof(tmp);

    *(int*)tmp = min;
    tmp += sizeof(tmp);

    *(int*)tmp = sec;
    tmp += sizeof(tmp);

    *(int*)tmp = msec;
    tmp += sizeof(tmp);

    memcpy(tmp,ba.data(),ba.size());
    m_pAVClient[audio_client]->SendData(0,buf,nPackSize);
    delete[] buf;
}
#include<QBuffer>


void Ckernel::slot_sendVideoFrame(QImage img)
{
//   qDebug()<<__func__ ;
//    //显示图片
//    slot_refreshVideo(m_id  ,img);
//    //压缩
//    //压缩图片从 RGB24 格式压缩到 JPEG 格式, 发送出去
//    QByteArray ba;
//    QBuffer qbuf(&ba); // QBuffer 与 QByteArray 字节数组建立联系
//    img.save( &qbuf , "JPEG" , 50 ); //将图片的数据写入 ba
//    //使用 ba 对象, 可以获取图片对应的缓冲区
//    //可以使用 ba.data() , ba.size()将缓冲区发送出去
//    //发送

//    ///视频数据帧
//    /// 成员描述
//    /// int type;
//    /// int userId;
//    /// int roomId;
//    /// int min;
//    /// int sec;
//    /// int msec;
//    /// QByteArray videoFrame;
//    int nPackSize = 6*sizeof(int)+ba.size();
//    char* buf = new char [nPackSize];
//    char* tmp = buf;

//    *(int*)tmp = DEF_PACK_VIDEO_FRAME;
//    tmp+= sizeof(int);

//    *(int*)tmp =  m_id;
//    tmp+= sizeof(int);

//    *(int*)tmp = m_roomid;
//    tmp+= sizeof(int);

//    //用于延迟过久 丢帧的参考时间
//    QTime tm=QTime::currentTime();
//    *(int*)tmp =  tm.minute();
//    tmp+= sizeof(int);

//    *(int*)tmp = tm.second();
//    tmp+= sizeof(int);
//    *(int*)tmp = tm.msec();
//    tmp+= sizeof(int);

//    memcpy(tmp,ba.data(),ba.size());

//    //将视频发送变为一个信号，放到另一个线程执行 todo
//    Q_EMIT SIG_SendVideo(buf,nPackSize);

 /**********************************************************/
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
void Ckernel::slot_SendVideo(char *buf, int nlen)
{
    char* tmp = buf;
    tmp+= sizeof(int);
    tmp+= sizeof(int);
    tmp+= sizeof(int);

    int min=*(int*)tmp;
    tmp += sizeof(int);
    int sec= *(int*)tmp;
    tmp += sizeof(int);
    int msec=*(int*)tmp;
    tmp+= sizeof(int);

    QTime ctm = QTime :: currentTime();
    QTime tm(ctm.hour(),min,sec,msec );

    //发送数据包延迟超过300MS 舍弃
    if(tm.msecsTo(ctm)>300)
    {
        qDebug()<<"send fail";
        delete[] buf;
        return;
    }
    //m_pClient->SendData(0,buf,nlen);
    m_pAVClient[video_client]->SendData(0,buf,nlen);
    delete[] buf;

}

void Ckernel::slot_sendVefCode(QByteArray ba)
{
    qDebug()<<__func__;
    //char* buf=ba.data();
    CJson json(ba);
    STRU_VEFCODE_RQ rq;
    strcpy_s(rq.m_tel,sizeof(rq.m_tel),json.json_get_string("tel").toStdString().c_str() );
    m_pClient->SendData(0,(char*)&rq,sizeof(rq));
}

void Ckernel::slot_commitVefCode(QByteArray ba)
{
    CJson json(ba);
    STRU_VEFCODE_RS rs;
    rs.m_userid=m_id;
    strcpy_s(rs.vefCode,sizeof (rs.vefCode),json.json_get_string("code").toStdString().c_str());
    strcpy_s(rs.m_tel,sizeof(rs.m_tel),json.json_get_string("tel").toStdString().c_str() );
    m_pClient->SendData(0,(char*)&rs,sizeof(rs));
}
