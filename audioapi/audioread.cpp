#include "audioread.h"
#include"QDebug"
AudioRead::AudioRead(QObject *parent) : QObject(parent)
{
     qDebug()<<__func__<<endl;
     //speex初始化
     speex_bits_init(&bits_enc);
     Enc_State = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));
     //设置压缩质量
    // #define SPEEX_QUALITY (8)
     //设置质量为 8（15kbs）
     int tmp = SPEEX_QUALITY;
     speex_encoder_ctl(Enc_State,SPEEX_SET_QUALITY,&tmp);
    //声卡采样格式 采样率8kHz  位宽16位，单声道

     format.setSampleRate(8000);
     format.setChannelCount(1);
     format.setSampleSize(16);
     format.setCodec("audio/pcm");
     format.setByteOrder(QAudioFormat::LittleEndian);
     format.setSampleType(QAudioFormat::UnSignedInt);
     QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
     if (!info.isFormatSupported(format)) {
         QMessageBox::information(NULL , "提示", "打开音频设备失败");
         format = info.nearestFormat(format);
    }
     m_audio_in =NULL;
     m_buffer_in = NULL;
     m_timer = new QTimer;
     connect(m_timer,SIGNAL(timeout()),this,SLOT(ReadMore()));
     m_audioState=stopped;

#ifdef USE_VAD
     //vadcreate
     if(0!=WebRtcVad_Create(&handle))
     {
         qDebug()<<"vad create fail";
     }
     //vad init
     if(0!= WebRtcVad_Init(handle))
     {
         qDebug()<<"vad init fail";
     }

#endif

}

AudioRead::~AudioRead()
{
    //delete Enc_State;
    if(m_timer)
    {
        m_timer->stop();
        delete m_timer;
        m_timer=NULL;
    }
    if(m_audio_in)
    {
        m_audio_in->stop();
        delete m_audio_in;
        m_audio_in=NULL;
    }
#ifdef USE_VAD
     WebRtcVad_Free(handle);
#endif
}

void AudioRead::ReadMore()
{
#ifdef USE_SPEEX
    if (!m_audio_in)
     return;
     QByteArray m_buffer(2048,0);
     qint64 len = m_audio_in->bytesReady();
     if (len < 640)
     {
        return;
     }
    qint64 l = m_buffer_in->read(m_buffer.data(), 640);
    QByteArray frame;

    //静音检测
#ifdef USE_VAD
    char* bufferdata = (char*)m_buffer.data();
    //每次操作 320字节
    for(int i=0;i<640;i+=320)
    {
        char* tmp = bufferdata+i;
        //vad 返回值 -1错误 0静音 1有声
        int res=WebRtcVad_Process(handle,8000,(int16_t*)tmp,160);
        if(0==res)
        {
            //清零这块空间==静音
           memset(tmp,0,320);
        }
    }
#endif
    //speex编码
    char bytes[800] = {0};
    int i = 0;
    float input_frame1[320];
    char buf[800] = {0};
    char* pInData = (char*)m_buffer.data() ;
    memcpy( buf , pInData , 640);
    int nbytes = 0;
    {
        short num = 0;
        for (i = 0;i < 160;i++)
        {
            num = (uint8_t)buf[2 * i] | (((uint8_t)buf[2 * i + 1]) << 8);
            input_frame1[i] = num;
            //num = m_buffer[2 * i] | ((short)(m_buffer[2 * i + 1]) << 8);
            //qDebug() << "float in" << num << input_frame1[i];
        }
        //压缩数据
        speex_bits_reset(&bits_enc);
        speex_encode(Enc_State,input_frame1,&bits_enc);
        nbytes = speex_bits_write(&bits_enc,bytes,800);
        frame.append(bytes,nbytes);
        //大端
        for (i = 0;i < 160;i++)
        {
            num = (uint8_t)buf[2 * i + 320] | (((uint8_t)buf[2 * i + 1 + 320]) << 8);
            input_frame1[i] = num;
        }
        //压缩数据
        speex_bits_reset(&bits_enc);
        speex_encode(Enc_State,input_frame1,&bits_enc);
        nbytes = speex_bits_write(&bits_enc,bytes,800);
        frame.append(bytes,nbytes);
        qDebug() << "nbytes = " << frame.size();
        Q_EMIT SIG_audioFrame (frame);
        return;
     }
//    frame.append(m_buffer.data(),640);
//    Q_EMIT SIG_audioFrame( frame );
#else
    if (!m_audio_in)
     return;
     QByteArray m_buffer(2048,0);
     qint64 len = m_audio_in->bytesReady();
     if (len < 640)
     {
     return;
    }
    qint64 l = m_buffer_in->read(m_buffer.data(), 640);
    QByteArray frame;
    frame.append(m_buffer.data(),640);
    Q_EMIT SIG_audioFrame( frame );
#endif
}

void AudioRead::pause()
{
     qDebug()<<__func__<<endl;
    if(m_audioState==playing  )
    {
        m_timer->stop();
        if(m_audio_in)
        {
            m_audio_in->stop();
            delete m_audio_in;
            m_audio_in=NULL;
        }
         m_audioState=pausing;
    }
}

void AudioRead::start()
{
     qDebug()<<__func__<<endl;
    if(m_audioState ==stopped || m_audioState==pausing  )
    {
        m_audio_in = new QAudioInput(format,this);
        m_buffer_in = m_audio_in->start();
        m_timer ->start(20);//20

    }
    m_audioState=playing;
}
