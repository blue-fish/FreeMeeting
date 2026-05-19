#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include<QVBoxLayout>
#include "usershow.h"
#include"QCloseEvent"
#include <QFile>
#include <QLabel>
#include <QStringList>

#define USE_OPUS (1)

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

    void SIG_SDLaudioPause();
    void SIG_SDLaudioStart();

    void SIG_videoStart();
    void SIG_videoPause();
    void SIG_screenStart();
    void SIG_screenPause();
    void SIG_setMoji(int moji);
    void SIG_recognitionChanged(bool enabled);

public:
    explicit RoomDialog(QWidget *parent = nullptr);
    ~RoomDialog();
public slots:
    void slot_setInfo(QString roomid);
    void slot_addUserShow(UserShow* user);
    void slot_refreshUser(int id, QImage img);
    void slot_removeUserShow(UserShow* user);
    //重载去除函数，根据id去除用户信息
    void slot_removeUserShow(int id);
    void slot_clearUserShow();
    void slot_setAudioCheck(bool check);
    void slot_setVideoCheck(bool check);

    void slot_setBigImageId(int id,QString name);
    void slot_setScreenCheck(bool check);
    void slot_setCaptionEnabled(bool check);
    void slot_appendCaption(int id, QString name, QString text, bool isFinal, qint64 timestamp);
    void slot_showAsrStatus(bool available, QString message);
private slots:
    void on_pb_close_clicked();

    void on_pb_quit_clicked();

    void closeEvent(QCloseEvent* event);

    void on_cb_audio_clicked();

    void on_cb_vedio_clicked();

    void on_cb_desk_clicked();

    void on_cb_moji_currentIndexChanged(int index);

    void on_cb_video_clicked();
    void on_cb_Recognition_clicked();

protected:
    void resizeEvent(QResizeEvent* event);

private:
    void setupCaptionPanel();
    void updateCaptionPanelGeometry();
    void ensureRecordFile();
    void writeRecordLine(const QString& name, const QString& text, qint64 timestamp);

    Ui::RoomDialog *ui;
    QVBoxLayout* m_mainLayout;
    std::map<int,UserShow*>m_mapIDToUserShow;
    QLabel* m_captionLabel;
    QStringList m_recentCaptions;
    QFile m_recordFile;
    bool m_captionEnabled;
};

#endif // ROOMDIALOG_H
