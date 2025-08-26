#include "VideoEncoder.h"
#include <QDebug>

VideoEncoder::VideoEncoder(QObject *parent)
    : QThread(parent)
    , m_codec(nullptr)
    , m_codecCtx(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_swsCtx(nullptr)
    , m_isInit(false)
    , m_stopFlag(false)
    , m_pts(0)
{
}

VideoEncoder::~VideoEncoder()
{
    stop();
    uninit();
}

void VideoEncoder::init(int width, int height, int fps, int bitrate)
{
    if(m_isInit) return;

    m_width = width;
    m_height = height;

    // 查找H264编码器
    m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!m_codec) {
        qDebug() << "找不到H264编码器";
        return;
    }

    // 分配编码器上下文
    m_codecCtx = avcodec_alloc_context3(m_codec);
    if (!m_codecCtx) {
        qDebug() << "无法分配编码器上下文";
        return;
    }

    // 设置编码参数
    m_codecCtx->codec_id = AV_CODEC_ID_H264;
    m_codecCtx->bit_rate = bitrate;
    m_codecCtx->width = width;
    m_codecCtx->height = height;
    m_codecCtx->time_base = {1, fps};
    m_codecCtx->framerate = {fps, 1};
    m_codecCtx->gop_size = 12;
    m_codecCtx->max_b_frames = 0;
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 设置编码预设
    av_opt_set(m_codecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_codecCtx->priv_data, "tune", "zerolatency", 0);

    // 打开编码器
    if (avcodec_open2(m_codecCtx, m_codec, nullptr) < 0) {
        qDebug() << "无法打开编码器";
        avcodec_free_context(&m_codecCtx);
        return;
    }

    // 分配帧
    m_frame = av_frame_alloc();
    m_frame->format = m_codecCtx->pix_fmt;
    m_frame->width = m_codecCtx->width;
    m_frame->height = m_codecCtx->height;
    av_frame_get_buffer(m_frame, 0);

    // 分配数据包
    m_packet = av_packet_alloc();

    m_isInit = true;
}

void VideoEncoder::uninit()
{
    if(!m_isInit) return;

    if(m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if(m_packet) {
        av_packet_free(&m_packet);
    }

    if(m_frame) {
        av_frame_free(&m_frame);
    }

    if(m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }

    m_isInit = false;
}

void VideoEncoder::addFrame(int userid, int roomid, const QImage& image)
{
    if(!m_isInit) return;

    EncodeTask task;
    task.userid = userid;
    task.roomid = roomid;
    task.image = image;
    task.pts = m_pts++;
    QMutexLocker locker(&m_mutex);
    m_taskQueue.enqueue(task);
}

void VideoEncoder::stop()
{
    m_stopFlag = true;
    if(isRunning()) {
        wait();
    }
}

void VideoEncoder::run()
{
    while(!m_stopFlag) {
        EncodeTask task;
        {
            QMutexLocker locker(&m_mutex);
            if(m_taskQueue.isEmpty()) {
                msleep(5);
                continue;
            }
            task = m_taskQueue.dequeue();
        }

        encodeFrame(task);
    }
}

bool VideoEncoder::encodeFrame(const EncodeTask& task)
{
    // 转换RGB到YUV
    uint8_t* yuv_buffer = nullptr;
    int yuv_size = rgbToYuv(task.image, &yuv_buffer);
    if(yuv_size <= 0) {
        return false;
    }

    // 填充YUV数据到frame
    av_image_fill_arrays(m_frame->data, m_frame->linesize,
                         yuv_buffer, AV_PIX_FMT_YUV420P,
                         m_codecCtx->width, m_codecCtx->height, 1);

    m_frame->pts = task.pts;

    // 编码
    int ret = avcodec_send_frame(m_codecCtx, m_frame);
    if (ret < 0) {
        av_free(yuv_buffer);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codecCtx, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            av_free(yuv_buffer);
            return false;
        }

        // 构造网络包时使用新结构
        int packLen = sizeof(STRU_VIDEO_H264_V2) + m_packet->size;
        char* buf = new char[packLen];

        STRU_VIDEO_H264_V2* pPack = (STRU_VIDEO_H264_V2*)buf;
        pPack->type = _DEF_PACK_VIDEO_H264;
        pPack->userid = task.userid;
        pPack->roomid = task.roomid;
        pPack->width = m_width;
        pPack->height = m_height;
        pPack->timestamp = 0;  // 将在发送时设置
        pPack->frameType = (m_packet->flags & AV_PKT_FLAG_KEY) ? 1 : 0;
        pPack->dataLen = m_packet->size;
        memcpy(pPack->data, m_packet->data, m_packet->size);

        emit SIG_sendVideoPacket(buf, packLen);

        av_packet_unref(m_packet);
    }

    av_free(yuv_buffer);
    return true;
}

int VideoEncoder::rgbToYuv(const QImage& image, uint8_t** yuv_buffer)
{
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

    if(!m_swsCtx) {
        m_swsCtx = sws_getContext(
            rgbImage.width(), rgbImage.height(), AV_PIX_FMT_RGB24,
            m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    int yuv_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                            m_codecCtx->width,
                                            m_codecCtx->height, 1);
    *yuv_buffer = (uint8_t*)av_malloc(yuv_size);

    uint8_t* src_data[4] = {(uint8_t*)rgbImage.bits(), nullptr, nullptr, nullptr};
    int src_linesize[4] = {rgbImage.bytesPerLine(), 0, 0, 0};

    uint8_t* dst_data[4];
    int dst_linesize[4];
    av_image_fill_arrays(dst_data, dst_linesize, *yuv_buffer,
                        AV_PIX_FMT_YUV420P, m_codecCtx->width,
                        m_codecCtx->height, 1);

    sws_scale(m_swsCtx, src_data, src_linesize, 0, rgbImage.height(),
              dst_data, dst_linesize);

    return yuv_size;
}
