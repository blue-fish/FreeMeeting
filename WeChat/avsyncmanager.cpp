#include "avsyncmanager.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>

AVSyncManager::AVSyncManager(int userid, QObject *parent)
    : QObject(parent)
    , m_userid(userid)
    , m_timer(new QTimer(this))
    , m_startTime(0)
    , m_firstFrameTime(0)
    , m_bufferTimeMs(150)  // 默认150ms缓冲
    , m_isRunning(false)
    , m_lastAudioTime(0)
    , m_lastVideoTime(0)
    , m_hasReceivedFirstFrame(false)  // 添加标志
{
    connect(m_timer, &QTimer::timeout, this, &AVSyncManager::processFrames);
    m_timer->setInterval(10);  // 10ms处理一次，100fps
}

AVSyncManager::~AVSyncManager()
{
    stop();
}

void AVSyncManager::addAudioFrame(int64_t timestamp, const QByteArray& data)
{
    QMutexLocker locker(&m_audioMutex);

    auto frame = std::make_shared<AudioFrame>(timestamp, m_userid, data);
    m_audioQueue.enqueue(frame);

    // 记录第一帧时间并启动计时器
    if (!m_hasReceivedFirstFrame) {
        m_hasReceivedFirstFrame = true;
        m_firstFrameTime = timestamp;
        m_elapsedTimer.start();
        qDebug() << "AVSyncManager - 收到第一帧音频，启动同步计时器 - 用户:" << m_userid << "时间戳:" << timestamp;
    }

    // 清理过期帧（超过5秒的旧帧）
    while (!m_audioQueue.isEmpty() &&
           m_audioQueue.head()->timestamp < timestamp - 5000) {
        m_audioQueue.dequeue();
    }
}

void AVSyncManager::addVideoFrame(int64_t timestamp, const QImage& image)
{
    qDebug() << "添加视频帧到同步队列 - 用户:" << m_userid << "时间戳:" << timestamp;

    QMutexLocker locker(&m_videoMutex);

    auto frame = std::make_shared<VideoFrame>(timestamp, m_userid, image);
    m_videoQueue.enqueue(frame);

    // 记录第一帧时间并启动计时器
    if (!m_hasReceivedFirstFrame) {
        m_hasReceivedFirstFrame = true;
        m_firstFrameTime = timestamp;
        m_elapsedTimer.start();
        qDebug() << "AVSyncManager - 收到第一帧视频，启动同步计时器 - 用户:" << m_userid << "时间戳:" << timestamp;
    }

    // 清理过期帧
    while (!m_videoQueue.isEmpty() &&
           m_videoQueue.head()->timestamp < timestamp - 5000) {
        m_videoQueue.dequeue();
    }
}

void AVSyncManager::start()
{
    if (m_isRunning) return;

    m_isRunning = true;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_timer->start();

    qDebug() << "AVSyncManager started for user" << m_userid << " (等待第一帧)";
}

void AVSyncManager::stop()
{
    if (!m_isRunning) return;

    m_isRunning = false;
    m_timer->stop();

    // 重置状态
    m_hasReceivedFirstFrame = false;
    m_firstFrameTime = 0;
    m_lastAudioTime = 0;
    m_lastVideoTime = 0;

    // 清空队列
    {
        QMutexLocker locker(&m_audioMutex);
        m_audioQueue.clear();
    }
    {
        QMutexLocker locker(&m_videoMutex);
        m_videoQueue.clear();
    }

    qDebug() << "AVSyncManager stopped for user" << m_userid;
}

void AVSyncManager::processFrames()
{
    if (!m_isRunning) return;

    // 如果还没有收到第一帧，不处理
    if (!m_hasReceivedFirstFrame) return;
    int64_t currentTime = getCurrentPlayTime();
    // 处理音频帧
    {
        QMutexLocker locker(&m_audioMutex);
        while (!m_audioQueue.isEmpty()) {
            auto frame = m_audioQueue.head();
            // 计算帧的延迟
            int64_t frameDelay = frame->timestamp - currentTime;
            if (frameDelay <= 0) {
                // 帧应该被播放
                m_audioQueue.dequeue();
                m_lastAudioTime = frame->timestamp;
                emit playAudioFrame(frame->data);
            } else if (frameDelay > 1000) {
                // 帧太早了（超过1秒），可能是时间戳错误
                m_audioQueue.dequeue();
                qDebug() << "Drop future audio frame, too early:" << frameDelay << "ms";
            } else {
                // 帧还没到播放时间，等待
                break;
            }
        }
    }

    // 处理视频帧
    {
        QMutexLocker locker(&m_videoMutex);
        while (!m_videoQueue.isEmpty()) {
            auto frame = m_videoQueue.head();
            // 计算帧的延迟
            int64_t frameDelay = frame->timestamp - currentTime;
            if (frameDelay <= 0) {
                // 帧的时间戳已到或已过，应该立即播放
                // 这也包含了处理"迟到"的帧，它们会被立即播放以尝试追赶
                m_videoQueue.dequeue();
                m_lastVideoTime = frame->timestamp;
                qDebug() << "播放视频帧 - 时间戳:" << frame->timestamp << "当前播放时间:" << currentTime << "延迟:" << -frameDelay << "ms";
                emit playVideoFrame(frame->image);
            } else if (frameDelay > 1000) {
                // 帧太早了（超过1秒），可能是时间戳错误，丢弃
                m_videoQueue.dequeue();
                qDebug() << "丢弃过早的视频帧, 领先:" << frameDelay << "ms";
            } else {
                // 帧还没到播放时间 (在1秒内)，等待
                // 由于队列是按时间戳排序的，后续的帧也不需要处理，直接退出循环
                break;
            }
        }
    }

    // 自适应调整缓冲时间
    adjustBufferTime();
}

int64_t AVSyncManager::getCurrentPlayTime() const
{
    if (!m_isRunning || !m_hasReceivedFirstFrame) return 0;

    // 计算从第一帧开始经过的时间
    int64_t elapsed = m_elapsedTimer.elapsed();

    // 当前播放时间 = 第一帧时间戳 + 经过的时间
    // 不需要减去缓冲时间，因为缓冲时间应该在判断是否播放时考虑
    return m_firstFrameTime + elapsed;
}

bool AVSyncManager::shouldPlayFrame(int64_t frameTime, int64_t currentTime) const
{
    // 如果帧时间小于等于当前播放时间，应该播放
    // 允许20ms的提前播放，避免卡顿
    return frameTime <= currentTime + 20;
}

void AVSyncManager::adjustBufferTime()
{
    // 根据队列长度动态调整缓冲时间
    int audioSize = getAudioQueueSize();
    int videoSize = getVideoQueueSize();
    int maxSize = std::max(audioSize, videoSize);

    if (maxSize > 10) {
        // 队列过长，减少缓冲时间
        m_bufferTimeMs = std::max(50, m_bufferTimeMs - 10);
    } else if (maxSize < 3) {
        // 队列过短，增加缓冲时间
        m_bufferTimeMs = std::min(500, m_bufferTimeMs + 10);
    }
}

int AVSyncManager::getAudioQueueSize()
{
    QMutexLocker locker(&m_audioMutex);
    return m_audioQueue.size();
}

int AVSyncManager::getVideoQueueSize()
{
    QMutexLocker locker(&m_videoMutex);
    return m_videoQueue.size();
}

int64_t AVSyncManager::getCurrentLatency()
{
    int64_t audioLatency = 0;
    int64_t videoLatency = 0;
    int64_t currentTime = getCurrentPlayTime();

    {
        QMutexLocker locker(&m_audioMutex);
        if (!m_audioQueue.isEmpty()) {
            audioLatency = m_audioQueue.head()->timestamp - currentTime;
        }
    }

    {
        QMutexLocker locker(&m_videoMutex);
        if (!m_videoQueue.isEmpty()) {
            videoLatency = m_videoQueue.head()->timestamp - currentTime;
        }
    }

    return std::max(audioLatency, videoLatency);
}
