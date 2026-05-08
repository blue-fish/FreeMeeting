#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include<QCloseEvent>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_loginCommit(QString tel, QString password);
    void  SIG_registerCommit(QString strTel,QString strPassword,QString strName);
    void SIG_sendVefCode(QByteArray ba);
    void SIG_commitVefCode(QByteArray ba);
    void SIG_close();
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    void closeEvent(QCloseEvent* event);
private slots:


    void on_pb_commit_clicked();

    void on_pb_clear_register_clicked();

    void on_pb_commit_register_clicked();

    void on_pb_clear_clicked();

    void on_pb_vefCommit_clicked();

    void on_pb_vefCode_clicked();


private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
