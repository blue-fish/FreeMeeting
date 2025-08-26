#ifndef WECHATDIALOG_H
#define WECHATDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include "deepseek.h"

QT_BEGIN_NAMESPACE
namespace Ui { class WeChatDialog; }
QT_END_NAMESPACE

class WeChatDialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_close();
    void SIG_createRoom();
    void SIG_joinRoom();
public:
    WeChatDialog(QWidget *parent = nullptr);
    ~WeChatDialog();

    void closeEvent(QCloseEvent *event);

    void setInfo(QString name,int icon=1);
private slots:
    void on_pb_create_clicked();

    void on_pb_join_clicked();

    void on_pb_AI_clicked();

private:
    Ui::WeChatDialog *ui;
    DeepSeek* w;
};
#endif // WECHATDIALOG_H
