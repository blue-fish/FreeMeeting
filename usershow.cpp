#include "usershow.h"
#include "ui_usershow.h"
#include"QDebug"
UserShow::UserShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserShow),m_defaultImg(":/tx/13.png")
{
    ui->setupUi(this);
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(slot_checkTimer()));

    m_lastTime = QTime::currentTime();
    m_timer.start(1000);

    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setStyleSheet("background: transparent;");
    ui->lb_name->setStyleSheet("color: #f0f0f0; font-size: 12px; padding: 6px 0 0 0; background: transparent;");
    ui->lb_name->setAlignment(Qt::AlignCenter);
    m_img = m_defaultImg;
}

UserShow::~UserShow()
{
    delete ui;
}

void UserShow::slot_setInfo(int id, QString name)
{
    qDebug()<<__func__<<name;
    m_id = id;
    m_userName = name;
    QString text = QString ("用户：%1").arg( m_userName);
    ui->lb_name->setText(text);
}

void UserShow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF backgroundRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    QLinearGradient gradient(backgroundRect.topLeft(), backgroundRect.bottomRight());
    gradient.setColorAt(0.0, QColor("#22252c"));
    gradient.setColorAt(1.0, QColor("#2f333a"));
    painter.setBrush(gradient);
    painter.setPen(QPen(QColor("#444b57"), 1));
    painter.drawRoundedRect(backgroundRect, 14, 14);

    int topMargin = ui->lb_name->height() + 20;
    QRect imageArea = rect().adjusted(16, topMargin, -16, -16);
    if (imageArea.isValid()) {
        QSize avatarSize = QSize(qMin(imageArea.width(), imageArea.height()), qMin(imageArea.width(), imageArea.height()));
        QRect avatarRect(
            imageArea.center().x() - avatarSize.width() / 2,
            imageArea.center().y() - avatarSize.height() / 2,
            avatarSize.width(),
            avatarSize.height());

        painter.setBrush(QColor("#1f242b"));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(avatarRect, 18, 18);
        QImage drawImg = m_img.isNull() ? m_defaultImg : m_img;
        if (!drawImg.isNull()) {
            QSize targetSize = avatarRect.size() * 0.92;
            QPixmap pixmap = QPixmap::fromImage(drawImg.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            QPoint imageTopLeft(
                avatarRect.x() + (avatarRect.width() - pixmap.width()) / 2,
                avatarRect.y() + (avatarRect.height() - pixmap.height()) / 2);

            QPainterPath clipPath;
            clipPath.addRoundedRect(avatarRect.adjusted(6, 6, -6, -6), 14, 14);
            painter.setClipPath(clipPath);
            painter.drawPixmap(imageTopLeft, pixmap);
            painter.setClipping(false);
        }

        painter.setPen(QPen(QColor(255, 255, 255, 60), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(avatarRect, 18, 18);
    }

    QWidget::paintEvent(event);
}

void UserShow::slot_setImage(QImage &img)
{
    m_lastTime = QTime::currentTime();
    m_img = img;
    this->update();
}

void UserShow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    Q_EMIT SIG_itemClicked(m_id,m_userName);
}

void UserShow::slot_checkTimer()
{
    if(m_lastTime.secsTo(QTime::currentTime())>5)
    {
        slot_setImage(m_defaultImg);
    }
}
