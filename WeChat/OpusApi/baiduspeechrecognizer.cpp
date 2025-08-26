#include "BaiduSpeechRecognizer.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonArray>
#include <cmath>

BaiduSpeechRecognizer::BaiduSpeechRecognizer(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_processTimer(new QTimer(this))
    , m_isRecognizing(false)
    , m_audioBufferSize(32000)
    , m_silenceDuration(0)
    , m_isSpeaking(false)
{
    connect(m_processTimer, &QTimer::timeout,
            this, &BaiduSpeechRecognizer::processAudioBuffer);
    m_processTimer->setInterval(250);
}

BaiduSpeechRecognizer::~BaiduSpeechRecognizer()
{
    stopRecognition();
}

void BaiduSpeechRecognizer::init(const QString& apiKey, const QString& secretKey)
{
    m_apiKey = apiKey;
    m_secretKey = secretKey;
    getAccessToken();
}

void BaiduSpeechRecognizer::addAudioData(const QByteArray& audioData)
{
    if (!m_isRecognizing) return;
    qDebug() << "BaiduSpeechRecognizer收到音频数据，大小:" << audioData.size();

    QMutexLocker locker(&m_bufferMutex);
    m_audioBuffer.enqueue(audioData);
    //qDebug() << "添加音频数据:" << audioData.size() << "字节";
}

void BaiduSpeechRecognizer::startRecognition()
{
    if (m_accessToken.isEmpty()) {
        emit recognitionError("请先初始化并获取Token");
        return;
    }

    m_isRecognizing = true;
    m_accumulatedAudio.clear();
    m_lastSoundTime = 0;
    m_hasSound = false;
    m_processTimer->start();
    qDebug() << "开始语音识别服务";
}

void BaiduSpeechRecognizer::stopRecognition()
{
    m_isRecognizing = false;
    m_processTimer->stop();

    // 处理剩余的音频
    if (!m_accumulatedAudio.isEmpty() && m_accumulatedAudio.size() >= MIN_AUDIO_LENGTH) {
        qDebug() << "停止时处理剩余音频:" << m_accumulatedAudio.size() << "字节";
        recognizeAudio(m_accumulatedAudio);
        m_accumulatedAudio.clear();
    }

    QMutexLocker locker(&m_bufferMutex);
    m_audioBuffer.clear();
}

// 修改静音检测，使其更准确
bool BaiduSpeechRecognizer::detectSilence(const QByteArray& audioData)
{
    const int16_t* samples = reinterpret_cast<const int16_t*>(audioData.data());
    int sampleCount = audioData.size() / sizeof(int16_t);
    if (sampleCount == 0) return true;
    // 计算RMS（均方根）而不是简单平均
    int64_t sumSquares = 0;
    for (int i = 0; i < sampleCount; ++i) {
        int64_t sample = samples[i];
        sumSquares += sample * sample;
    }
    double rms = std::sqrt(static_cast<double>(sumSquares) / sampleCount);

    static double adaptiveThreshold = SILENCE_THRESHOLD;

    bool isSilent = rms < adaptiveThreshold;

    // 调试输出
    if (sampleCount > 100) {  // 避免太频繁的输出
        qDebug() << "音频RMS:" << rms << "阈值:" << adaptiveThreshold << "静音:" << isSilent;
    }
    return isSilent;
}

void BaiduSpeechRecognizer::processAudioBuffer()
{
    QByteArray currentAudio;

    {
        QMutexLocker locker(&m_bufferMutex);
        while (!m_audioBuffer.isEmpty()) {
            currentAudio.append(m_audioBuffer.dequeue());
        }
    }

    // 处理新到达的音频数据
    if (!currentAudio.isEmpty()) {
        bool isSilent = detectSilence(currentAudio);

        if (!isSilent) {
            // 检测到声音：累积音频并更新状态
            m_accumulatedAudio.append(currentAudio);
            m_lastSoundTime = QDateTime::currentMSecsSinceEpoch();
            m_hasSound = true;
            qDebug() << "检测到声音，累积音频大小:" << m_accumulatedAudio.size();
        } else {
            // 当前是静音，但如果之前有声音，仍然需要累积（可能是短暂停顿）
            if (m_hasSound) {
                m_accumulatedAudio.append(currentAudio);
                qDebug() << "检测到静音片段，但仍在累积";
            }
        }
    }

    // 检查是否需要发送识别（不管有没有新数据都要检查！）
    if (m_hasSound && !m_accumulatedAudio.isEmpty()) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 silenceDuration = currentTime - m_lastSoundTime;

        qDebug() << "静音时长:" << silenceDuration << "ms, 累积音频:"
                 << m_accumulatedAudio.size() << "字节";

        // 触发识别的条件：
        // 1. 静音超过阈值
        // 2. 音频长度达到最大值
        // 3. 即使没有新数据，但静音时间过长
        bool shouldRecognize = false;
        QString reason;

        if (silenceDuration >= SILENCE_DURATION_THRESHOLD) {
            shouldRecognize = true;
            reason = "静音超时";
        } else if (m_accumulatedAudio.size() >= MAX_AUDIO_LENGTH) {
            shouldRecognize = true;
            reason = "音频过长";
        }

        if (shouldRecognize && m_accumulatedAudio.size() >= MIN_AUDIO_LENGTH) {
            qDebug() << "触发识别，原因:" << reason
                     << "，音频时长:" << m_accumulatedAudio.size() / 32000.0 << "秒";

            recognizeAudio(m_accumulatedAudio);

            // 重置状态
            m_accumulatedAudio.clear();
            m_hasSound = false;
            m_lastSoundTime = 0;
        } else if (shouldRecognize && !m_accumulatedAudio.isEmpty()) {
            qDebug() << "音频太短(" << m_accumulatedAudio.size() << "字节)，继续等待";
            // 不清空，继续累积
        }
    }

    // 3. 防护机制：处理异常情况
    if (!m_accumulatedAudio.isEmpty()) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        // 如果累积了音频但是超过10秒都没有新的声音，强制发送
        if (m_lastSoundTime > 0 && (currentTime - m_lastSoundTime) > 10000) {
            qDebug() << "警告：超过10秒没有新声音，强制发送";
            if (m_accumulatedAudio.size() >= MIN_AUDIO_LENGTH) {
                recognizeAudio(m_accumulatedAudio);
            }
            m_accumulatedAudio.clear();
            m_hasSound = false;
            m_lastSoundTime = 0;
        }
    }
}




void BaiduSpeechRecognizer::recognizeAudio(const QByteArray& audioData)
{
    qDebug() << "发送识别请求，音频长度:" << audioData.size() / 32000.0 << "秒";
    QString url = "https://vop.baidu.com/server_api";
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");  // 避免连接问题
    QJsonObject json;
    json["format"] = "pcm";
    json["rate"] = 16000;
    json["channel"] = 1;
    json["cuid"] = "qt_demo_user";
    json["token"] = m_accessToken;
    json["speech"] = QString(audioData.toBase64());
    json["len"] = audioData.size();
    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    qDebug() << "请求数据大小:" << postData.size();

    QNetworkReply* reply = m_networkManager->post(request, postData);
    // 只保留一个连接，删除重复的
    connect(reply, &QNetworkReply::finished, [this, reply, audioData]() {  // 捕获audioData用于调试
        qDebug() << "收到百度API响应";

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "响应大小:" << responseData.size();
            qDebug() << "百度API响应:" << responseData;
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isNull()) {
                qDebug() << "JSON解析失败";
                emit recognitionError("JSON解析失败");
                reply->deleteLater();
                return;
            }

            QJsonObject obj = doc.object();
            int errNo = obj["err_no"].toInt();
            QString errMsg = obj["err_msg"].toString();
            qDebug() << "错误码:" << errNo << "错误信息:" << errMsg;

            if (errNo == 0) {
                QJsonArray results = obj["result"].toArray();
                if (!results.isEmpty()) {
                    QString text = results[0].toString();
                    qDebug() << "识别成功:" << text;
                    emit recognitionResult(text);
                } else {
                    qDebug() << "识别结果为空";
                }
            } else {
                QString errorMsg = QString("识别错误[%1]: %2").arg(errNo).arg(errMsg);
                qDebug() << errorMsg;
                emit recognitionError(errorMsg);
            }
        } else {
            QString errorMsg = QString("网络错误[%1]: %2")
                .arg(reply->error())
                .arg(reply->errorString());
            qDebug() << errorMsg;
            qDebug() << "HTTP状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emit recognitionError(errorMsg);
        }
        reply->deleteLater();
    });

    // 错误处理
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << "网络请求错误:" << error << reply->errorString();
    });
}

void BaiduSpeechRecognizer::getAccessToken()
{
    QString tokenUrl = QString("https://aip.baidubce.com/oauth/2.0/token?"
                              "grant_type=client_credentials&"
                              "client_id=%1&"
                              "client_secret=%2")
                              .arg(m_apiKey)
                              .arg(m_secretKey);

    QNetworkRequest request(tokenUrl);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            m_accessToken = obj["access_token"].toString();
            qDebug() << "获取Token成功:" << m_accessToken;
        } else {
            QString errorMsg = "获取Token失败: " + reply->errorString();
            qDebug() << errorMsg;
            emit recognitionError(errorMsg);
        }
        reply->deleteLater();
    });
}
