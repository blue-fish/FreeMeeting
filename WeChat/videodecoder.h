#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include "packdef.h"
#include "VideoApi/common.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    explicit VideoDecoder(int userid, QObject *parent = nullptr);
    ~VideoDecoder();

    void init(int width, int height);
    void uninit();
    void addPacket(char* data, int len, int64_t pts);
    void stop();

signals:
    // 修改信号，添加时间戳参数
    void SIG_frameDecoded(int userid, QImage image, qint64 timestamp);

protected:
    void run() override;

private:
    bool decodePacket(char* data, int len, int64_t timestamp);

public:
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }


private:
    int m_userid;

    // 解码器相关
    AVCodec* m_codec;
    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVFrame* m_frameRGB;
    AVPacket* m_packet;
    SwsContext* m_swsCtx;
    uint8_t* m_rgbBuffer;

    // 数据队列
    struct PacketData {
        char* data;
        int len;
        int64_t pts;
    };
    QQueue<PacketData> m_packetQueue;
    QMutex m_mutex;

    // 控制变量
    bool m_isInit;
    bool m_stopFlag;
    int m_width;
    int m_height;
};

#endif // VIDEODECODER_H
