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

    QString text = QString ("用户名：%1").arg( m_userName);
    ui->lb_name->setText(text);
}
//重绘
void UserShow::paintEvent(QPaintEvent *event)
{

    //画黑色背景
     QPainter painter(this);
     painter.setBrush(Qt::black);
     painter.drawRect(-1,0,this->width()+1,this->height());

     //画图片
      if( m_img.size().height()<= 0 ) return;
      // 加载图片用 QImage , 画图使用 QPixmap
      // 图片缩放 scaled
      QPixmap pixmap = QPixmap::fromImage( m_img.scaled( QSize( this->width() ,
     this->height() - ui->lb_name->height()), Qt::KeepAspectRatio ));
      //画的位置
      int x = this->width() - pixmap.width();
      x = x /2;
      int y = this->height() - pixmap.height() - ui->lb_name->height();
      y = ui->lb_name->height() + y/2;
      painter.drawPixmap( QPoint(x,y) , pixmap );
      painter.end();
}

void UserShow::slot_setImage(QImage &img)
{
    //qDebug()<<__func__ ;
    //每次获得画面的更新时间
    m_lastTime = QTime::currentTime();
    m_img = img;
    this->update();//触发重绘
}

void UserShow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    Q_EMIT SIG_itemClicked(m_id,m_userName);
}

void UserShow::slot_checkTimer()
{
    //定时检测 接收界面是否超时
    if(m_lastTime.secsTo(QTime::currentTime())>5)
    {
        //设为默认 画面
        slot_setImage(m_defaultImg);

    }
}
