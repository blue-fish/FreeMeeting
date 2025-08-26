#include "roomdialog.h"
#include "ui_roomdialog.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <math.h>

RoomDialog::RoomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomDialog)
{
    ui->setupUi(this);

    m_mainLayout=new QVBoxLayout;

    m_mainLayout->setContentsMargins(0,0,0,0);
    m_mainLayout->setSpacing(5);

    //设置一个垂直布局的画布,向这里添加控件
    ui->wdg_list->setLayout(m_mainLayout);

    //测试
//    for(int i=0;i<6;i++){
//        UserShow* user=new UserShow;
//        user->slot_setInfo(i+1,QString("测试%1").arg(i+1));
//        slot_addUserShow(user);
    //    }

    // 创建字幕控件
    m_subtitleWidget = new SubtitleWidget(this);
    m_subtitleWidget->setVisible(false);  // 默认隐藏

    // 设置布局
    if (ui->danmakuContainer) {
        ui->danmakuContainer->setAttribute(Qt::WA_TranslucentBackground, true);
        ui->danmakuContainer->setStyleSheet("QWidget#danmakuContainer { background-color: transparent; border: none; }");
        QVBoxLayout* layout = new QVBoxLayout(ui->danmakuContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_subtitleWidget);
    }

    // 确保checkbox默认是未选中状态
    ui->cb_Recognition->setChecked(false);

}

RoomDialog::~RoomDialog()
{
    delete ui;
}

void RoomDialog::slot_setInfo(QString roomid)
{
    QString title=QString("房间号:%1").arg(roomid);
    setWindowTitle(title);
    ui->lb_title->setText(title);
}

void RoomDialog::slot_addUserShow(UserShow *user)
{
    // 为右侧小窗口设置固定大小
    user->setFixedSize(200, 180);

    m_mainLayout->addWidget(user);
    m_mapIDToUserShow[user->m_id]=user;
}


void RoomDialog::slot_refreshUser(int id, QImage &img)
{
    if(ui->wdg_userShow->m_id==id){
        ui->wdg_userShow->slot_setImage(img);
    }
    if(m_mapIDToUserShow.count(id)>0){
        UserShow* user=m_mapIDToUserShow[id];
        user->slot_setImage(img);
    }
}

void RoomDialog::slot_removeUserShow(UserShow *user)
{
    user->hide();
    m_mainLayout->removeWidget(user);
}

void RoomDialog::slot_removeUserShow(int id)
{
    if(m_mapIDToUserShow.count(id)>0){
        UserShow* user=m_mapIDToUserShow[id];
        slot_removeUserShow(user);
    }
}

void RoomDialog::slot_setAudioCheck(bool check)
{
    ui->cb_audio->setChecked(check);
}

void RoomDialog::slot_setVideoCheck(bool check)
{
    ui->cb_video->setChecked(check);
}

void RoomDialog::slot_setScreenCheck(bool check)
{
    ui->cb_desk->setChecked(check);
}

void RoomDialog::slot_clearUserShow()
{
    for(auto ite=m_mapIDToUserShow.begin();ite!=m_mapIDToUserShow.end();ite++){
        slot_removeUserShow(ite->second);
    }
}

void RoomDialog::slot_setBigImgID(int id, QString name)
{
    ui->wdg_userShow->slot_setInfo(id,name);
}

//退出房间
void RoomDialog::on_pb_close_clicked()
{
    this->close();
}

//退出房间
void RoomDialog::on_pb_quit_clicked()
{
    this->close();
}

void RoomDialog::closeEvent(QCloseEvent *event)
{
    if(QMessageBox::question(this,"退出提示","是否退出会议?")==QMessageBox::Yes){
        //发送退出房间信号
        Q_EMIT SIG_close();
        event->accept();
        return;
    }
    event->ignore();
}

//开启关闭 音频
void RoomDialog::on_cb_audio_clicked()
{
    if(ui->cb_audio->isChecked()){
        //ui->cb_audio->setChecked(false);
        Q_EMIT SIG_audioStart();
    }else{
        //ui->cb_audio->setChecked(true);
        Q_EMIT SIG_audioPause();
    }
}

//开启关闭 视频
void RoomDialog::on_cb_video_clicked()
{
    if(ui->cb_video->isChecked()){
        ui->cb_desk->setChecked(false);
        Q_EMIT SIG_screenPause();
        Q_EMIT SIG_videoStart();
    }else{
        Q_EMIT SIG_videoPause();
    }
}


void RoomDialog::on_cb_desk_clicked()
{
    if(ui->cb_desk->isChecked()){
        ui->cb_video->setChecked(false);
        Q_EMIT SIG_videoPause();
        Q_EMIT SIG_screenStart();
    }else{
        Q_EMIT SIG_screenPause();
    }
}


void RoomDialog::on_cb_moji_currentIndexChanged(int index)
{
    Q_EMIT SIG_setMoji(index);
}


/////////////////////////////////字幕//////////////////////


void RoomDialog::on_cb_Recognition_stateChanged(int arg1)
{
    if(ui->cb_Recognition->isChecked()){
        // 开启字幕显示和语音识别
        if(m_subtitleWidget) {
            m_subtitleWidget->setVisible(true);
        }
        emit SIG_startRecognition();
        qDebug() << "开启字幕功能";
    }else{
        // 关闭字幕显示和语音识别
        if(m_subtitleWidget) {
            m_subtitleWidget->setVisible(false);
        }
        emit SIG_stopRecognition();
        qDebug() << "关闭字幕功能";
    }
}




void RoomDialog::onRecognitionResult(const QString& text)
{
    qDebug() << "Dialog收到识别结果：" << text;


    // 再添加字幕
    if (!text.isEmpty()) {
        qDebug() << "添加字幕：" << m_currentSpeaker << ":" << text;
        m_subtitleWidget->addSubtitle(m_currentSpeaker, text);

        // 验证字幕是否显示
        qDebug() << "字幕控件可见性:" << m_subtitleWidget->isVisible();
        qDebug() << "字幕控件大小:" << m_subtitleWidget->size();
    }
}

void RoomDialog::onRecognitionError(const QString& error)
{
    qDebug() << "Dialog收到识别错误：" << error;
}
