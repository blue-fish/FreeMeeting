#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(int userid, QObject *parent)
    : QThread(parent)
    , m_userid(userid)
    , m_codec(nullptr)
    , m_codecCtx(nullptr)
    , m_frame(nullptr)
    , m_frameRGB(nullptr)
    , m_packet(nullptr)
    , m_swsCtx(nullptr)
    , m_rgbBuffer(nullptr)
    , m_isInit(false)
    , m_stopFlag(false)
{
}

VideoDecoder::~VideoDecoder()
{
    stop();
    uninit();
}

void VideoDecoder::init(int width, int height)
{
    if(m_isInit) return;

    m_width = width;
    m_height = height;

    // 查找H264解码器
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec) {
        qDebug() << "找不到H264解码器";
        return;
    }

    // 分配解码器上下文
    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (!m_codecCtx) {
        qDebug() << "无法分配解码器上下文";
        return;
    }

    // 设置解码参数
    m_codecCtx->width = width;
    m_codecCtx->height = height;
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // 打开解码器
    if (avcodec_open2(m_codecCtx, m_codec, nullptr) < 0) {
        qDebug() << "无法打开解码器";
        avcodec_free_context(&m_codecCtx);
        return;
    }

    // 分配帧
    m_frame = av_frame_alloc();
    m_frameRGB = av_frame_alloc();

    // 分配RGB缓冲区
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    m_rgbBuffer = (uint8_t*)av_malloc(numBytes);
    av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize,
                        m_rgbBuffer, AV_PIX_FMT_RGB24, width, height, 1);

    // 分配数据包
    m_packet = av_packet_alloc();

    // 启动解码线程
    start();

    m_isInit = true;
}

void VideoDecoder::uninit()
{
    if(!m_isInit) return;

    // 清理队列
    {
        QMutexLocker locker(&m_mutex);
        while(!m_packetQueue.isEmpty()) {
            PacketData data = m_packetQueue.dequeue();
            delete[] data.data;
        }
    }

    if(m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if(m_rgbBuffer) {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }

    if(m_packet) {
        av_packet_free(&m_packet);
    }

    if(m_frameRGB) {
        av_frame_free(&m_frameRGB);
    }

    if(m_frame) {
        av_frame_free(&m_frame);
    }

    if(m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }

    m_isInit = false;
}

void VideoDecoder::addPacket(char* data, int len, int64_t pts)
{
    if(!m_isInit) return;

    PacketData packet;
    packet.data = new char[len];
    memcpy(packet.data, data, len);
    packet.len = len;
    packet.pts = pts;

    QMutexLocker locker(&m_mutex);
    m_packetQueue.enqueue(packet);
}

void VideoDecoder::stop()
{
    m_stopFlag = true;
    if(isRunning()) {
        wait();
    }
}

void VideoDecoder::run()
{
    while(!m_stopFlag) {
        PacketData data;
        {
            QMutexLocker locker(&m_mutex);
            if(m_packetQueue.isEmpty()) {
                msleep(5);
                continue;
            }
            data = m_packetQueue.dequeue();
        }

        // 传递时间戳到解码函数
        decodePacket(data.data, data.len, data.pts);
        delete[] data.data;
    }
}

bool VideoDecoder::decodePacket(char* data, int len, int64_t timestamp)
{
    qDebug() << "解码视频帧 - 用户:" << m_userid << "时间戳:" << timestamp;

    m_packet->data = (uint8_t*)data;
    m_packet->size = len;

    int ret = avcodec_send_packet(m_codecCtx, m_packet);
    if (ret < 0) {
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            return false;
        }

        // YUV转RGB
        if(!m_swsCtx) {
            m_swsCtx = sws_getContext(
                m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_RGB24,
                SWS_BICUBIC, nullptr, nullptr, nullptr);
        }

        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0,
                  m_codecCtx->height, m_frameRGB->data, m_frameRGB->linesize);

        // 创建QImage
        QImage image(m_rgbBuffer, m_codecCtx->width, m_codecCtx->height,
                     QImage::Format_RGB888);

        // 发送解码后的帧，包含时间戳
        emit SIG_frameDecoded(m_userid, image.copy(), static_cast<qint64>(timestamp));
    }

    return true;
}
