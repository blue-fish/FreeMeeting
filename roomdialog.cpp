#include "roomdialog.h"
#include "ui_roomdialog.h"
#include"QMessageBox"
#include"QDebug"
RoomDialog::RoomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomDialog)
{
    ui->setupUi(this);
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setContentsMargins(0,0,0,0);
    m_mainLayout->setSpacing(5);
    //设置垂直布局的画布
    ui->wdg_list->setLayout(m_mainLayout);

}

RoomDialog::~RoomDialog()
{
    delete ui;
}

void RoomDialog::slot_setInfo(QString roomid)
{
    QString title = QString("房间号：%1").arg(roomid);
    //qDebug()<<title;
    setWindowTitle(title);
    ui->lb_title->setText(title);
}

void RoomDialog::slot_addUserShow(UserShow *user)
{
    m_mainLayout->addWidget(user);
    m_mapIDToUserShow[user->m_id] = user;
}

void RoomDialog::slot_refreshUser(int id, QImage img)
{
   // qDebug()<<__func__;
    //预览图id 与 刷新的图片 id 一致就刷新 预览图
    if(ui->wdg_userShow->m_id==id)
    {
        ui->wdg_userShow->slot_setImage(img);
    }
    if(m_mapIDToUserShow.count(id)>0)
    {
             UserShow* user = m_mapIDToUserShow[id];
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
    if(m_mapIDToUserShow.count(id)>0)
    {
        UserShow* user = m_mapIDToUserShow[id];
        slot_removeUserShow(user);
    }
}

void RoomDialog::slot_clearUserShow()
{
    for(auto ite = m_mapIDToUserShow.begin();
        ite!= m_mapIDToUserShow.end();++ite)
    {
        slot_removeUserShow(ite->second);

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
void RoomDialog::slot_setBigImageId(int id,QString name)
{

    ui->wdg_userShow->slot_setInfo(id,name);
}
//退出房间
void RoomDialog::on_pb_close_clicked()
{
    this->close();
}


void RoomDialog::on_pb_quit_clicked()
{
    this->close();
}

void RoomDialog::closeEvent(QCloseEvent * event)
{
    if(QMessageBox::question(this,"退出提示","是否退出会议？")==QMessageBox::Yes)
    {
        //退出房间
        Q_EMIT SIG_close();
        event->accept();
        return ;
    }
    event ->ignore();
}

//开启 关闭 音频
void RoomDialog::on_cb_audio_clicked()
{
#ifdef USE_OPUS
    if(ui->cb_audio->isChecked())
    {
         Q_EMIT SIG_SDLaudioStart();

    }else
    {
         Q_EMIT SIG_SDLaudioPause();
    }
#else
    if(ui->cb_audio->isChecked())
    {
         Q_EMIT SIG_audioStart();

    }else
    {
         Q_EMIT SIG_audioPause();
    }
#endif
}

//开启 关闭视频
void RoomDialog::on_cb_vedio_clicked()
{

}


void RoomDialog::on_cb_desk_clicked()
{
    if(ui->cb_desk->isChecked())
    {
        ui->cb_video->setChecked(false);
         Q_EMIT SIG_videoPause();
         Q_EMIT SIG_screenStart();

    }else
    {
         Q_EMIT SIG_screenPause();

    }
}


void RoomDialog::on_cb_moji_currentIndexChanged(int index)
{
     qDebug()<<__func__;
    Q_EMIT SIG_setMoji(index);
}


void RoomDialog::on_cb_video_clicked()
{
    if(ui->cb_video->isChecked())
    {
        ui->cb_desk->setChecked(false);
         Q_EMIT SIG_screenPause();
         Q_EMIT SIG_videoStart();

    }else
    {
         Q_EMIT SIG_videoPause();

    }
}

