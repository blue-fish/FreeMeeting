#ifndef USERSHOW_H
#define USERSHOW_H

#include <QWidget>
#include <QPaintEvent>
#include <QImage>
#include<QMouseEvent>

#include<QTimer>
#include<QTime>
namespace Ui {
class UserShow;
}

class UserShow : public QWidget
{
    Q_OBJECT

public:
    explicit UserShow(QWidget *parent = nullptr);
    ~UserShow();
signals:
    void SIG_itemClicked(int id ,QString name);

public slots:
    void slot_setInfo(int id,QString name);
    void paintEvent(QPaintEvent* event);
    void slot_setImage(QImage& img);

    void mousePressEvent(QMouseEvent* event);

    void slot_checkTimer();

    // 添加这个函数来处理大小变化
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::UserShow *ui;

    int m_id;
    QString m_userName;
    QImage m_img;
    friend class RoomDialog;

    QImage m_defaultImg;
    QTimer m_timer;
    QTime m_lastTime;
};

#endif // USERSHOW_H
