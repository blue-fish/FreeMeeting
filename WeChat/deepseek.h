#ifndef DEEPSEEK_H
#define DEEPSEEK_H

#include <QWidget>
#include<QNetworkAccessManager>
#include <QJsonObject> // 需要包含
#include <QList>     // 需要包含

namespace Ui {
class DeepSeek;
}

class DeepSeek : public QWidget
{
    Q_OBJECT

public:
    explicit DeepSeek(QWidget *parent = nullptr);
    ~DeepSeek();

private slots:
    void on_pb_commit_clicked();

private:
    Ui::DeepSeek *ui;
    QNetworkAccessManager *manager;
    QList<QJsonObject> conversationHistory; // <<--- 添加对话历史存储
    QString currentAssistantMessageBuffer; // <<--- 用于拼接流式输出的助手回复
};

#endif // DEEPSEEK_H
