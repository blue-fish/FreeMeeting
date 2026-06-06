#ifndef SCREENREAD_H
#define SCREENREAD_H

#include <QObject>
#include <QTimer>
#include<QApplication>
#include<QDesktopWidget>
#include<QScreen>

class ScreenRead : public QObject
{
    Q_OBJECT
public:
    explicit ScreenRead(QObject *parent = nullptr);
    ~ScreenRead();
signals:
    void SIG_getScreenFrame(QImage img,qint64 time);

public slots:
    void slot_getSreenFrame();
    void slot_openVideo();
    void slot_closeVideo();
private:
    QTimer * m_timer;

};

#endif // SCREENREAD_H
