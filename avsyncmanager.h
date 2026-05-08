#ifndef AVSYNCMANAGER_H
#define AVSYNCMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QElapsedTimer>
#include <memory>
#include <QImage>

// 音视频帧基类
struct MediaFrame {
    int64_t timestamp;
    int userid;
    enum FrameType { AUDIO_FRAME, VIDEO_FRAME } type;
    QByteArray data;

    MediaFrame(int64_t ts, int uid, FrameType t)
        : timestamp(ts), userid(uid), type(t) {}

    virtual ~MediaFrame() = default;
};

// 音频帧
struct AudioFrame : public MediaFrame {
    AudioFrame(int64_t ts, int uid, const QByteArray& d)
        : MediaFrame(ts, uid, MediaFrame::AUDIO_FRAME) {
        data = d;
    }
};

// 视频帧
struct VideoFrame : public MediaFrame {
    QImage image;
    VideoFrame(int64_t ts, int uid, const QImage& img)
        : MediaFrame(ts, uid, MediaFrame::VIDEO_FRAME), image(img) {}
};

class AVSyncManager : public QObject
{
    Q_OBJECT
public:
    explicit AVSyncManager(int userid, QObject *parent = nullptr);
    ~AVSyncManager();

    // 添加音视频帧
    void addAudioFrame(int64_t timestamp, const QByteArray& data);
    void addVideoFrame(int64_t timestamp, const QImage& image);

    // 开始/停止同步播放
    void start();
    void stop();

    // 设置缓冲时间（毫秒）
    void setBufferTime(int ms) { m_bufferTimeMs = ms; }

    // 获取统计信息
    int getAudioQueueSize();
    int getVideoQueueSize();
    int64_t getCurrentLatency();

signals:
    void playAudioFrame(const QByteArray& data);
    void playVideoFrame(const QImage& image);

private slots:
    void processFrames();

private:
    int m_userid;
    QTimer* m_timer;
    QMutex m_audioMutex;
    QMutex m_videoMutex;

    // 音视频缓冲队列
    QQueue<std::shared_ptr<AudioFrame>> m_audioQueue;
    QQueue<std::shared_ptr<VideoFrame>> m_videoQueue;

    // 同步控制
    int64_t m_startTime;        // 开始播放时间
    int64_t m_firstFrameTime;   // 第一帧时间戳
    int m_bufferTimeMs;         // 缓冲时间
    bool m_isRunning;
    bool m_hasReceivedFirstFrame;  // 添加：是否收到第一帧标志

    // 统计信息
    QElapsedTimer m_elapsedTimer;
    int64_t m_lastAudioTime;
    int64_t m_lastVideoTime;

    // 自适应缓冲
    void adjustBufferTime();
    int64_t getCurrentPlayTime() const;
    bool shouldPlayFrame(int64_t frameTime, int64_t currentTime) const;
};

#endif // AVSYNCMANAGER_H
