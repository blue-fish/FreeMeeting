#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include "packdef.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

// 编码任务结构
struct EncodeTask {
    int userid;
    int roomid;
    QImage image;
    int64_t pts;
};

class VideoEncoder : public QThread
{
    Q_OBJECT
public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder();

    void init(int width, int height, int fps = 15, int bitrate = 400000);
    void uninit();
    void addFrame(int userid, int roomid, const QImage& image);
    void stop();

signals:
    void SIG_sendVideoPacket(char* buf, int len);

protected:
    void run() override;

private:
    bool encodeFrame(const EncodeTask& task);
    int rgbToYuv(const QImage& image, uint8_t** yuv_buffer);

private:
    // 编码器相关
    AVCodec* m_codec;
    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVPacket* m_packet;
    SwsContext* m_swsCtx;

    // 任务队列
    QQueue<EncodeTask> m_taskQueue;
    QMutex m_mutex;

    // 控制变量
    bool m_isInit;
    bool m_stopFlag;
    int m_width;
    int m_height;
    int64_t m_pts;
};

#endif // VIDEOENCODER_H
