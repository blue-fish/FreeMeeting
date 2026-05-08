#include "sdlaudiowrite.h"

SDLAudioWrite::SDLAudioWrite(QObject *parent) : QObject(parent)
{
    //解码器初始化
    int err;
    decoder = opus_decoder_create(48000, 1, &err);

    //设置音频
    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_zero(wantedSpec);
    wantedSpec.freq = 48000;
    wantedSpec.format = AUDIO_S16LSB;
    wantedSpec.channels = 1;
    wantedSpec.samples = 960;
    wantedSpec.callback = audioCallback;
    wantedSpec.userdata = this; //回调函数传参
    //打开设备
    // 打开播放设备 第一个为空意味着 默认设备 , 第二个
    dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
    if (dev == 0) {
    qDebug() << "Failed to open audio device: " << SDL_GetError() ;
    SDL_Quit();
    return ;
    }
    //默认播放
    slot_openAudio();
}

SDLAudioWrite::~SDLAudioWrite()
{
    // 停止播放音频
    SDL_PauseAudioDevice(dev, 1);
    // 关闭音频设备和 SDL
    SDL_CloseAudio();
    SDL_Quit();

}
void SDLAudioWrite::slot_playAudioFrame(QByteArray recvBuffer)
{
     qDebug()<<__func__<<endl;
    if( !m_isOpen ) return;
    std::lock_guard<std::mutex> lck( m_mutex );
    m_audioQueue.emplace_back ( recvBuffer );
}

void SDLAudioWrite::audioCallback(void *userdata, Uint8 *stream, int len)
{
    // qDebug()<<"play thread:"<<QThread::currentThreadId();
    SDLAudioWrite * audio = (SDLAudioWrite *)userdata;
    memset( stream , 0 , len );
    if( !audio->m_audioQueue.empty() )
    {
        QByteArray recvBuffer = audio->m_audioQueue.front();
        {
            std::lock_guard<std::mutex> lck( audio->m_mutex );
            audio->m_audioQueue.pop_front();
        }
        //解码
        opus_int16 decodedData[4096];
        int frameSizeDecoded = opus_decode(audio->decoder, (const uchar*)recvBuffer.data(),
        recvBuffer.size(), decodedData,sizeof(decodedData)/sizeof(decodedData[0]), 0);
        if (frameSizeDecoded < 0) {
            return;
        }
//        解码函数 opus_decode 第一个参数 【解码器】 第二个参数要【解码的缓冲区】, 第三参数【缓冲区大小】, 第四参数【输出的
//        缓冲区】, 第五参数【缓冲区大小】, 第六参数【选项, 默认值, 表示内容丢失也要解码】

        //混音
//        SDL_MixAudioFormat( stream , (uint8_t*)recvBuffer.data() , AUDIO_S16LSB
//                            , recvBuffer.size() , 100);
        SDL_MixAudioFormat( stream , (uint8_t*)decodedData  , AUDIO_S16LSB
                            ,  frameSizeDecoded*sizeof(opus_int16), 100);
    }
}
//计算比特率 压缩比
//播放音频收到的包大小，120-150
//以120为例
//120*50 = 6000字节 - 6500字节/s  6kB/s --->(1B=8b) 8*6kbps 48kbps
//1920 : 120 压缩率10倍以上

//而 speex的压缩率 640->76 压缩率不到10倍
