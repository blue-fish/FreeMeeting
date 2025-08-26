#include "deepseek.h"
#include "ui_deepseek.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QSslSocket> // 包含 qInfo 才需要，或者直接使用 qDebug

DeepSeek::DeepSeek(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeepSeek)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager;
    qDebug() << "SSL Library Build Version:" << QSslSocket::sslLibraryBuildVersionString();

    // 初始化对话历史，可以先加入 system message
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are a helpful assistant. Please answer in Chinese if the user asks in Chinese."; // 可以指定语言偏好
    conversationHistory.append(systemMsg);
}

DeepSeek::~DeepSeek()
{
    delete ui;
}

void DeepSeek::on_pb_commit_clicked()
{
    //获取文本框内容
    QString userInputText = ui->te_input->toPlainText();
    if (userInputText.trimmed().isEmpty()) { // 避免发送空消息
        return;
    }

    ui->te_input->clear();

    ui->te_output->insertPlainText("-->");
    ui->te_output->insertPlainText(userInputText);
    ui->te_output->insertPlainText("\n\n");

    // 1. 将用户的新消息添加到对话历史
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userInputText;
    conversationHistory.append(userMsg);

    // 2. 构建 API 请求的 messages 数组
    QJsonArray messagesForApi;
    for (const QJsonObject& msg : conversationHistory) {
        messagesForApi.append(msg);
    }

    QNetworkRequest request;
    request.setUrl(QUrl("https://api.deepseek.com/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    // !!! 警告：请勿将 API Key 硬编码到生产代码中。考虑使用配置文件或环境变量。
    request.setRawHeader("Authorization", "Bearer sk-374fac0fc4834dc9b4149c216bb4d857");


    QJsonObject requestBody;
    requestBody["messages"] = messagesForApi; // <<--- 使用包含历史的 messages
    requestBody["model"] = "deepseek-chat";
    requestBody["max_tokens"] = 2048;
    requestBody["stream"] = true;
    requestBody["temperature"] = 0.7; // 可以适当调整 temperature

    // 清空上一次流式回复的缓冲区
    currentAssistantMessageBuffer.clear();

    //发送请求
    QNetworkReply* reply = manager->post(request, QJsonDocument(requestBody).toJson());

    //处理数据
    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        while (reply->canReadLine()) {
            QString line = reply->readLine().trimmed();

            if (line.startsWith("data: ")) {
                line.remove(0, 6);
                if (line == "[DONE]") { // 流结束标记
                    return;
                }
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);

                if (error.error == QJsonParseError::NoError) {
                    QJsonObject jsonObj = doc.object();
                    if (jsonObj.contains("choices")) {
                        QJsonArray choices = jsonObj["choices"].toArray();
                        if (!choices.isEmpty()) {
                            QJsonObject firstChoice = choices.first().toObject();
                            if (firstChoice.contains("delta")) {
                                QJsonObject delta = firstChoice["delta"].toObject();
                                if (delta.contains("content")) {
                                    QString contentPart = delta["content"].toString();
                                    if (!contentPart.isEmpty()) {
                                        ui->te_output->moveCursor(QTextCursor::End);
                                        ui->te_output->insertPlainText(contentPart);
                                        currentAssistantMessageBuffer.append(contentPart); // <<--- 累积助手回复
                                    }
                                }
                            }
                        }
                    }
                } else {
                    qWarning() << "JSON Parse Error:" << error.errorString() << "in line:" << line;
                }
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        ui->te_output->moveCursor(QTextCursor::End);
        ui->te_output->insertPlainText("\n\n"); // 视觉分隔

        if (reply->error() == QNetworkReply::NoError) {
            // 3. 将完整的助手回复添加到对话历史
            if (!currentAssistantMessageBuffer.isEmpty()) {
                QJsonObject assistantMsg;
                assistantMsg["role"] = "assistant";
                assistantMsg["content"] = currentAssistantMessageBuffer;
                conversationHistory.append(assistantMsg);
                currentAssistantMessageBuffer.clear();
            }
        } else {
            // 处理网络错误或API错误
            qWarning() << "Network Error:" << reply->errorString() << " (Code: " << reply->error() << ")"; // 可以加上错误码
            ui->te_output->insertPlainText("Error: " + reply->errorString() + "\n\n");
            // 如果出错，可能需要从 conversationHistory 中移除最后一条用户消息，
            // 因为这次交互失败了。
            if (!conversationHistory.isEmpty() && conversationHistory.last()["role"].toString() == "user") {
                // 这是一个考虑点，根据你的需求决定是否移除
                // conversationHistory.removeLast();
            }
        }
        reply->deleteLater();
    });
}

