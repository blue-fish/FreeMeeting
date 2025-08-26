#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>

#include <QVBoxLayout>
#include"usershow.h"
#include "BaiduSpeechRecognizer.h"
#include "AudioResampler.h"
// 替换弹幕相关的声明
#include "subtitlewidget.h"

namespace Ui {
class RoomDialog;
}

class RoomDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_close();
    void SIG_audioPause();
    void SIG_audioStart();
    void SIG_videoPause();
    void SIG_videoStart();
    void SIG_screenPause();
    void SIG_screenStart();
    void SIG_setMoji(int moji);

    void SIG_startRecognition();
    void SIG_stopRecognition();
public:
    explicit RoomDialog(QWidget *parent = nullptr);
    ~RoomDialog();
public slots:
    void slot_setInfo(QString roomid);

    void slot_addUserShow(UserShow* user);

    void slot_refreshUser(int id,QImage& img);

    void slot_removeUserShow(UserShow* user);

    void slot_removeUserShow(int id);

    void slot_setAudioCheck(bool check);

    void slot_setVideoCheck(bool check);

    void slot_setScreenCheck(bool check);

    void slot_clearUserShow();

    void slot_setBigImgID(int id,QString name);
private slots:
    void on_pb_close_clicked();

    void on_pb_quit_clicked();

    void closeEvent(QCloseEvent* event);
    void on_cb_audio_clicked();

    void on_cb_video_clicked();

    void on_cb_desk_clicked();

    void on_cb_moji_currentIndexChanged(int index);

    void on_cb_Recognition_stateChanged(int arg1);

private:
    Ui::RoomDialog *ui;
    QVBoxLayout* m_mainLayout;
    std::map<int,UserShow*> m_mapIDToUserShow;

    ///////////////////////////////字幕////////////////////

public slots:
    void onRecognitionResult(const QString& text);
    void onRecognitionError(const QString& error);
    // 设置当前说话人
    void setCurrentSpeaker(const QString& speaker) {
        m_currentSpeaker = speaker;
    }

    // 添加远程用户的字幕
    void addRemoteSubtitle(const QString& speaker, const QString& text) {
        qDebug()<<"1111"<<endl;
        m_subtitleWidget->addSubtitle(speaker, text);
    }


private:

    SubtitleWidget* m_subtitleWidget;  // 替换 m_danmakuWidget
    QString m_currentSpeaker;  // 当前说话人
};

#endif // ROOMDIALOG_H
