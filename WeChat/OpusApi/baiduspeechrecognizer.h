

//#endif // BAIDUSPEECHRECOGNIZER_H
#ifndef BAIDUSPEECHRECOGNIZER_H
#define BAIDUSPEECHRECOGNIZER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QElapsedTimer>

class BaiduSpeechRecognizer : public QObject
{
    Q_OBJECT
public:
    explicit BaiduSpeechRecognizer(QObject *parent = nullptr);
    ~BaiduSpeechRecognizer();

    void init(const QString& apiKey, const QString& secretKey);
    void addAudioData(const QByteArray& audioData);
    void startRecognition();
    void stopRecognition();

signals:
    void recognitionResult(const QString& text);
    void recognitionError(const QString& error);

private slots:
    void processAudioBuffer();
private:
    void getAccessToken();
    void recognizeAudio(const QByteArray& audioData);
    bool detectSilence(const QByteArray& audioData);

private:
    //网络相关的操作，主要是发送HTTP请求和接收HTTP响应
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QString m_secretKey;
    QString m_accessToken;

    QTimer* m_processTimer;
    QQueue<QByteArray> m_audioBuffer;
    QMutex m_bufferMutex;

    // 新增成员
    QByteArray m_accumulatedAudio;  // 累积的音频数据
    QElapsedTimer m_silenceTimer;    // 静音计时器
    int m_silenceDuration;           // 静音持续时间（毫秒）
    bool m_isSpeaking;               // 是否正在说话

    bool m_isRecognizing;
    int m_audioBufferSize;

    //QElapsedTimer m_silenceTimer;    // 静音计时器
    QElapsedTimer m_lastAudioTimer;  // 新增：最后接收音频时间计时器
    //int m_silenceDuration;           // 静音持续时间（毫秒）

    //QByteArray m_accumulatedAudio;  // 累积的音频数据
        qint64 m_lastSoundTime;          // 最后一次检测到声音的时间戳
        bool m_hasSound;                 // 是否检测到过声音

    // 可配置参数 - 调整这些值以获得更好的效果
    static constexpr int MIN_AUDIO_LENGTH = 16000 * 0.2;      // 最小1秒音频
    static constexpr int MAX_AUDIO_LENGTH = 16000 * 10;     // 最大10秒音频
    static constexpr int SILENCE_THRESHOLD = 100;           // 静音阈值
    static constexpr int SILENCE_DURATION_THRESHOLD = 800; // 0.8秒静音表示句子结束

};

#endif // BAIDUSPEECHRECOGNIZER_H
