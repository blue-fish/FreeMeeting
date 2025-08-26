#include "usershow.h"
#include "ui_usershow.h"
#include<QPainter>

UserShow::UserShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserShow),m_defaultImg(":/images/novideo.png")  //(":/bq/000.png")
{
    ui->setupUi(this);

    // 移除固定大小限制
    setMinimumSize(QSize(0, 0));
    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    connect(&m_timer,SIGNAL(timeout())
            ,this,SLOT(slot_checkTimer()));

    m_lastTime=QTime::currentTime();

    m_timer.start(1000);
}

UserShow::~UserShow()
{
    delete ui;
}

//设置
void UserShow::slot_setInfo(int id, QString name)
{
    m_id=id;
    m_userName=name;

    QString text =QString("用户名:%1").arg(m_userName);
    ui->lb_name->setText(text);
}

void UserShow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // 调整标签位置和大小
    if(ui->lb_name) {
        int labelHeight = 35;
        ui->lb_name->setGeometry(0, height() - labelHeight, width(), labelHeight);
    }
}

void UserShow::paintEvent(QPaintEvent *event)
{
    //重绘事件
    // 画黑背景
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 填充黑色背景
    painter.fillRect(rect(), Qt::black);

    // 如果没有图像，显示默认图标
    if(m_img.size().height() <= 0) {
        painter.setPen(QPen(Qt::white, 2));
        int fontSize = qMin(width(), height()) / 8; // 根据窗口大小调整字体
        painter.setFont(QFont("Microsoft YaHei", fontSize));
        painter.drawText(rect().adjusted(0, 0, 0, -ui->lb_name->height()),
                        Qt::AlignCenter, "📹");
        return;
    }

    // 计算可用空间（减去底部用户名高度）
    int labelHeight = ui->lb_name->height();
    int availableHeight = this->height() - labelHeight;
    int availableWidth = this->width();

    // 保持宽高比缩放图片
    QImage scaledImg = m_img.scaled(availableWidth, availableHeight,
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 计算居中位置
    int x = (availableWidth - scaledImg.width()) / 2;
    int y = (availableHeight - scaledImg.height()) / 2;

    // 绘制图片
    painter.drawImage(x, y, scaledImg);
}

void UserShow::slot_setImage(QImage &img)
{
    m_img=img;
    //每次获得画面更新时间
    m_lastTime=QTime::currentTime();

    this->update();
}

void UserShow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    Q_EMIT SIG_itemClicked(m_id,m_userName);
}

void UserShow::slot_checkTimer()
{
    //定时检测 接收界面是否超时
    if(m_lastTime.secsTo(QTime::currentTime())>5){
        //设为默认画面
        slot_setImage(m_defaultImg);
    }
}
