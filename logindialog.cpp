#include "logindialog.h"
#include "ui_logindialog.h"
#include <QRegExp>
#include<QMessageBox>
#include<QDebug>
//#include "cjson.h"
#include<packdef.h>
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("注册&登录");
    ui->tw_page->setCurrentIndex(1);//第一时间显示注册
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
     Q_EMIT SIG_close();
}
//提交登录
void LoginDialog::on_pb_commit_clicked()
{
    qDebug()<<__func__;
       QString strTel=ui->le_tel->text();
       QString strPassword = ui->le_password->text();
       //校验
       //0.非空校验
       QString telTmp=strTel;
       QString passTmp=strPassword;
       if(telTmp.remove(" ").isEmpty() || passTmp.remove(" ").isEmpty())
       {
           QMessageBox ::about(this,"提示","手机号,密码,昵称等不能为空格");
           return;
       }
       //1.校验手机号
       QRegExp reg("^1[3-9][0-9]\{6,9\}$");
       bool res = reg.exactMatch(strTel);
       if(!res)
       {
           QMessageBox::about(this,"提示","手机号格式错误");
                   return;
       }
       //校验密码 长度不能超过20
       if(strPassword.length()>20 || strPassword.length()<6)
       {
            QMessageBox::about(this,"提示","密码长度不合法");
            return;
       }

       Q_EMIT SIG_loginCommit(strTel,strPassword);

}

//登录清空
void LoginDialog::on_pb_clear_clicked()
{
    ui->le_tel->setText("");
    ui->le_password->setText("");
}
//注册清空
void LoginDialog::on_pb_clear_register_clicked()
{
    ui->le_tel_register->setText("");
    ui->le_password_register->setText("");
    ui->le_confirm_register->setText("");
    ui->le_name_register->setText("");
}

//注册提交
void LoginDialog::on_pb_commit_register_clicked()
{
     qDebug()<<__func__;
    QString strTel=ui->le_tel_register->text();
    QString strPassword = ui->le_password_register->text();
    QString strConfirm = ui->le_confirm_register->text();
    QString strName = ui->le_name_register->text();
    //校验
    //0.非空校验
    QString telTmp=strTel;
    QString passTmp=strPassword;
    //QString confirmTmp = strConfirm;
    QString nameTmp = strName;
    if(telTmp.remove(" ").isEmpty() || passTmp.remove(" ").isEmpty() || nameTmp.remove(" ").isEmpty())
    {
        QMessageBox ::about(this,"提示","手机号密码不能为空格");
        return;
    }
    //1.校验手机号
    QRegExp reg("^1[3-9][0-9]\{6,9\}$");
    bool res = reg.exactMatch(strTel);
    if(!res)
    {
        QMessageBox::about(this,"提示","手机号格式错误");
                return;
    }
    //校验密码 长度不能超过20
    if(strPassword.length()>20 || strPassword.length()<6)
    {
         QMessageBox::about(this,"提示","密码长度不合法");
         return;
    }
    if(strConfirm!=strPassword)
    {
        QMessageBox::about(this,"提示","两次密码不一致");
        return;
    }
    Q_EMIT SIG_registerCommit(strTel,strPassword,strName);
}



void LoginDialog::on_pb_vefCommit_clicked()
{
    qDebug()<<__func__;

    QString strTel =ui->le_vefTel->text();
    //1.校验手机号
    QRegExp reg("^1[3-9][0-9]\{6,9\}$");
    bool res = reg.exactMatch(strTel);
    if(!res)
    {
        QMessageBox::about(this,"提示","手机号格式错误");
                return;
    }
    //校验密码 长度不能超过20
    QString strPassword=ui->le_vefCode->text();
    if(strPassword.length()!=4)
    {
         QMessageBox::about(this,"提示","密码长度不合法");
         return;
    }
     CJson json;
    json.json_add_value("tel",strTel.toStdString().c_str());
    json.json_add_value("code", strPassword.toStdString().c_str());
    json.json_add_value("type",DEF_PACK_VEFCODE_RS);
    QByteArray ba=json.json_to_string();
    Q_EMIT SIG_commitVefCode(ba);
}


void LoginDialog::on_pb_vefCode_clicked()
{
    qDebug()<<__func__;

    QString strTel =ui->le_vefTel->text();
    //1.校验手机号
    QRegExp reg("^1[3-9][0-9]\{6,9\}$");
    bool res = reg.exactMatch(strTel);
    if(!res)
    {
        QMessageBox::about(this,"提示","手机号格式错误");
                return;
    }
    CJson json;
    //json.json_add_value("vefType","10001");
    json.json_add_value("tel",ui->le_vefTel->text().toStdString().c_str());
    QByteArray ba=json.json_to_string();
    Q_EMIT SIG_sendVefCode(ba);

}

