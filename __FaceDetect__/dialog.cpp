#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setWindowTitle("人脸检测");

    m_captureSample=new CaptureSample;

    connect(m_captureSample,SIGNAL(SIG_getOneImage(QImage&))
            ,this,SLOT(slot_getOneImage(QImage&)));
}

Dialog::~Dialog()
{
    delete ui;
}


void Dialog::on_pb_start_clicked()
{
    m_captureSample->slot_openCapture();
}


void Dialog::on_pb_close_clicked()
{
    m_captureSample->slot_closeCapture();
}


void Dialog::on_pb_quit_clicked()
{
    m_captureSample->slot_closeCapture();
    this->close();
}

void Dialog::slot_getOneImage(QImage &img)
{
    if(img.isNull()) return;

    QPixmap pixmap=QPixmap::fromImage(img.scaled(ui->lb_video->size(),Qt::KeepAspectRatio));

    ui->lb_video->setPixmap(pixmap);
}

