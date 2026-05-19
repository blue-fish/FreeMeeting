#include "roomdialog.h"
#include "ui_roomdialog.h"
#include"QMessageBox"
#include"QDebug"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QResizeEvent>
#include <QTextStream>
RoomDialog::RoomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomDialog),
    m_captionLabel(nullptr),
    m_captionEnabled(false)
{
    ui->setupUi(this);
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setContentsMargins(0,0,0,0);
    m_mainLayout->setSpacing(5);
    //设置垂直布局的画布
    ui->wdg_list->setLayout(m_mainLayout);
    setupCaptionPanel();
    ui->cb_Recognition->setChecked(false);

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

void RoomDialog::slot_setCaptionEnabled(bool check)
{
    m_captionEnabled = check;
    ui->cb_Recognition->setChecked(check);
    if (!m_captionLabel) {
        return;
    }
    m_captionLabel->setVisible(check);
    if (!check) {
        m_recentCaptions.clear();
        m_captionLabel->clear();
    }
}

void RoomDialog::slot_appendCaption(int id, QString name, QString text, bool isFinal, qint64 timestamp)
{
    if (!m_captionEnabled || text.trimmed().isEmpty()) {
        return;
    }
    if (name.trimmed().isEmpty()) {
        name = QString("User %1").arg(id);
    }

    const QString line = QString("%1: %2").arg(name, text.trimmed());
    if (isFinal) {
        if (m_recentCaptions.isEmpty() || m_recentCaptions.last() != line) {
            m_recentCaptions << line;
        }
        writeRecordLine(name, text.trimmed(), timestamp);
    } else {
        if (m_recentCaptions.isEmpty()) {
            m_recentCaptions << line;
        } else {
            m_recentCaptions.last() = line;
        }
    }

    while (m_recentCaptions.size() > 5) {
        m_recentCaptions.removeFirst();
    }
    if (m_captionLabel) {
        m_captionLabel->setText(m_recentCaptions.join("\n"));
        updateCaptionPanelGeometry();
    }
}

void RoomDialog::slot_showAsrStatus(bool available, QString message)
{
    if (!m_captionEnabled || available || !m_captionLabel) {
        return;
    }
    m_captionLabel->setVisible(true);
    m_captionLabel->setText(QString("[ASR unavailable] %1").arg(message));
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

void RoomDialog::on_cb_Recognition_clicked()
{
    slot_setCaptionEnabled(ui->cb_Recognition->isChecked());
    Q_EMIT SIG_recognitionChanged(ui->cb_Recognition->isChecked());
}

void RoomDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updateCaptionPanelGeometry();
}

void RoomDialog::setupCaptionPanel()
{
    m_captionLabel = new QLabel(ui->videoArea);
    m_captionLabel->setObjectName("captionLabel");
    m_captionLabel->setWordWrap(true);
    m_captionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_captionLabel->setStyleSheet(
        "QLabel {"
        "background-color: rgba(0, 0, 0, 170);"
        "color: white;"
        "border-radius: 8px;"
        "padding: 10px 14px;"
        "font-size: 16px;"
        "}");
    m_captionLabel->hide();
    updateCaptionPanelGeometry();
}

void RoomDialog::updateCaptionPanelGeometry()
{
    if (!m_captionLabel || !ui || !ui->videoArea) {
        return;
    }
    const int margin = 20;
    const int height = 120;
    const int width = qMax(280, ui->videoArea->width() - margin * 2);
    const int y = qMax(margin, ui->videoArea->height() - height - margin);
    m_captionLabel->setGeometry(margin, y, width, height);
    m_captionLabel->raise();
}

void RoomDialog::ensureRecordFile()
{
    if (m_recordFile.isOpen()) {
        return;
    }
    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.exists("meeting_records")) {
        dir.mkdir("meeting_records");
    }
    const QString fileName = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
    m_recordFile.setFileName(dir.filePath("meeting_records/" + fileName));
    m_recordFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void RoomDialog::writeRecordLine(const QString& name, const QString& text, qint64 timestamp)
{
    ensureRecordFile();
    if (!m_recordFile.isOpen()) {
        return;
    }
    const QDateTime time = timestamp > 0
        ? QDateTime::fromMSecsSinceEpoch(timestamp)
        : QDateTime::currentDateTime();
    QTextStream out(&m_recordFile);
    out.setCodec("UTF-8");
    out << "[" << time.toString("HH:mm:ss") << "] "
        << name << ": " << text << "\n";
    out.flush();
}

