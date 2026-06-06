#include "avsyncmanager.h"
#include "sdlaudiowrite.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>

AVSyncManager::AVSyncManager(int userid, SDLAudioWrite* audioWrite, QObject *parent)
    : QObject(parent)
    , m_userid(userid)
    , m_timer(new QTimer(this))
    , m_startTime(0)
    , m_firstFrameTime(0)
    , m_bufferTimeMs(150)
    , m_isRunning(false)
    , m_lastAudioTime(0)
    , m_lastVideoTime(0)
    , m_hasReceivedFirstFrame(false)
    , m_buffering(true)
    , m_audioWrite(audioWrite)
    , m_clockOffset(0)
{
    connect(m_timer, &QTimer::timeout, this, &AVSyncManager::processFrames);
    m_timer->setInterval(10);
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

    if (!m_hasReceivedFirstFrame) {
        m_hasReceivedFirstFrame = true;
        m_firstFrameTime = timestamp;
        m_clockOffset = timestamp - m_audioWrite->getPlayedDurationMs();
        m_buffering = true;
        m_elapsedTimer.start();
        qDebug() << "AVSyncManager - first audio frame, user:" << m_userid << "ts:" << timestamp;
    }

    while (!m_audioQueue.isEmpty() &&
           m_audioQueue.head()->timestamp < timestamp - 5000) {
        m_audioQueue.dequeue();
    }
}

void AVSyncManager::addVideoFrame(int64_t timestamp, const QImage& image)
{
    QMutexLocker locker(&m_videoMutex);
    auto frame = std::make_shared<VideoFrame>(timestamp, m_userid, image);
    m_videoQueue.enqueue(frame);

    if (!m_hasReceivedFirstFrame) {
        m_hasReceivedFirstFrame = true;
        m_firstFrameTime = timestamp;
        m_clockOffset = timestamp - m_audioWrite->getPlayedDurationMs();
        m_buffering = true;
        m_elapsedTimer.start();
        qDebug() << "AVSyncManager - first video frame, user:" << m_userid << "ts:" << timestamp;
    }

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
    qDebug() << "AVSyncManager started for user" << m_userid;
}

void AVSyncManager::stop()
{
    if (!m_isRunning) return;
    m_isRunning = false;
    m_timer->stop();

    m_hasReceivedFirstFrame = false;
    m_firstFrameTime = 0;
    m_lastAudioTime = 0;
    m_lastVideoTime = 0;
    m_clockOffset = 0;
    m_buffering = true;

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
    if (!m_hasReceivedFirstFrame) return;

    int64_t currentTime = getCurrentPlayTime();

    // Initial buffering is wall-clock based; the SDL playback clock starts
    // after the first queued audio frame is handed to SDLAudioWrite.
    if (m_buffering) {
        if (m_elapsedTimer.elapsed() < m_bufferTimeMs) {
            return;
        }
        m_buffering = false;
        qDebug() << "AVSyncManager - buffering complete, user:" << m_userid;
    }

    {
        QMutexLocker locker(&m_audioMutex);
        while (!m_audioQueue.isEmpty()) {
            auto frame = m_audioQueue.head();
            int64_t frameDelay = frame->timestamp - currentTime;

            if (frameDelay <= -200) {
                m_audioQueue.dequeue();
                qDebug() << "Drop stale audio frame, delay:" << -frameDelay << "ms, user:" << m_userid;
            } else if (shouldPlayFrame(frame->timestamp, currentTime)) {
                m_audioQueue.dequeue();
                m_lastAudioTime = frame->timestamp;
                emit playAudioFrame(frame->data);
            } else if (frameDelay > 5000) {
                m_audioQueue.dequeue();
                qDebug() << "Drop future audio frame, ahead:" << frameDelay << "ms, user:" << m_userid;
            } else {
                break;
            }
        }
    }

    currentTime = getCurrentPlayTime();

    {
        QMutexLocker locker(&m_videoMutex);
        while (!m_videoQueue.isEmpty()) {
            auto frame = m_videoQueue.head();
            int64_t frameDelay = frame->timestamp - currentTime;

            if (frameDelay <= -200) {
                // Frame > 200ms behind audio clock -> stale, discard
                m_videoQueue.dequeue();
                qDebug() << "Drop stale video frame, delay:" << -frameDelay << "ms, user:" << m_userid;
            } else if (frameDelay <= 0) {
                // Frame is due, play immediately
                m_videoQueue.dequeue();
                m_lastVideoTime = frame->timestamp;
                emit playVideoFrame(frame->image);
            } else if (frameDelay > 5000) {
                // Frame way too far in the future -> timestamp error, discard
                m_videoQueue.dequeue();
                qDebug() << "Drop future video frame, ahead:" << frameDelay << "ms, user:" << m_userid;
            } else {
                // Frame is in the future, wait
                break;
            }
        }
    }

    adjustBufferTime();
}

int64_t AVSyncManager::getCurrentPlayTime() const
{
    if (!m_isRunning || !m_hasReceivedFirstFrame) return 0;
    if (!m_audioWrite) return m_firstFrameTime + m_elapsedTimer.elapsed();
    if (m_lastAudioTime == 0 && m_audioWrite->getPlayedDurationMs() == 0) {
        int64_t elapsedAfterBuffer = m_elapsedTimer.elapsed() - m_bufferTimeMs;
        return m_firstFrameTime + std::max<int64_t>(0, elapsedAfterBuffer);
    }
    return m_audioWrite->getPlayedDurationMs() + m_clockOffset; //返回的是转换到音频播放时间的系统时间戳
}

bool AVSyncManager::shouldPlayFrame(int64_t frameTime, int64_t currentTime) const
{
    return frameTime <= currentTime + 20;
}

void AVSyncManager::adjustBufferTime()
{
    int audioSize = getAudioQueueSize();
    int videoSize = getVideoQueueSize();
    int maxSize = std::max(audioSize, videoSize);

    if (maxSize > 10) {
        m_bufferTimeMs = std::max(50, m_bufferTimeMs - 10);
    } else if (maxSize < 3) {
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
