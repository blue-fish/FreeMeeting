#include "sdlaudioread.h"

SDLAudioRead::SDLAudioRead(QObject *parent) : QObject(parent)
{
    //编码初始化
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug() << "Failed to initialize SDL: " << SDL_GetError() ;
        return ;
    }
    // 初始化 Opus 编码器和解码器
    int err;
    //参数：1频率，2通道数（声道），
    //3支持的类型（语音通话voip,audio（音频）,低延迟），4错误
    encoder = opus_encoder_create(48000, 1,
                OPUS_APPLICATION_VOIP, &err);


    //sdl设置
    // 设置音频规格
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 48000; //采样率 48000Hz
    desiredSpec.format = AUDIO_S16LSB; //16 位 小端存储
    desiredSpec.channels = 1; //单声道
    desiredSpec.samples = 960; // 回调函数 每 960 个点 回调一次
    // 因为 1s 是 48000 点 那么 20ms采集一次，一次采集到的是 960 个点，1s/20ms=50,50*960=48000
    desiredSpec.callback = audioCallback; //回调函数
    desiredSpec.userdata = this; //回调函数传参（参数就是当前对象）

    //打开设备
    // 打开录音设备 第一个为空意味着 默认设备
    dev = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0);
    if (dev == 0) {
        qDebug() << "Failed to open audio device: " << SDL_GetError() ;
        SDL_Quit();
        return ;
    }
#ifdef USE_VAD
      //vad 创建
    if(0!=WebRtcVad_Create(&handle))
    {
        qDebug()<<"vad create fail";
    }
      //vad初始化
    if(0!= WebRtcVad_Init(handle))
    {
        qDebug()<<"vad init fail";
    }
#endif
}

SDLAudioRead::~SDLAudioRead()
{
    // 停止采集音频
    SDL_PauseAudioDevice(dev, 1);
    // 关闭音频设备并清理
    SDL_CloseAudio();
    SDL_Quit();
#ifdef USE_VAD
    WebRtcVad_Free(handle);
#endif
    opus_encoder_destroy(encoder);

}

void SDLAudioRead::slot_sendAudioFrame( QByteArray &bt )
{
    Q_EMIT SIG_sendAudioFrame(bt);
}

//音频回调函数作用：将采集到的数据 编码发送, 此处稍后加编码
void SDLAudioRead::audioCallback(void *userdata, Uint8 *stream, int len)
{
   // qDebug()<<"sample thread:"<<QThread::currentThreadId();
    SDLAudioRead * audio = (SDLAudioRead *)userdata;
    if( len < 1920 ) return;

    //静音检测
    //如果是就返回
#ifdef USE_VAD
    int16_t *audioData = reinterpret_cast<int16_t*>(stream);
    int frameSize = 480; // 160 样本/帧（假设 10ms@16kHz，需根据实际调整）
    int numFrames = len / (frameSize * sizeof(int16_t));
    for(int i=0;i<960;i+=480)
    {
        int res = WebRtcVad_Process(audio->handle, 48000, audioData + i , frameSize);
           if (res <= 0) { // 静音或错误
               return;
           }
    }
#endif

    unsigned char opusData[4096];
    int nbBytes = opus_encode( audio->encoder, (const opus_int16*)( stream ),960, opusData,
    sizeof(opusData));
    if (nbBytes < 0)//返回的nbBytes是长度
    {
        qDebug() << "Opus encode error:" << opus_strerror(nbBytes);
        return;
    }
    //opus_encode 是编码函数 第一个参数是上面定义的编码器 , 第二个参数是要编码的缓冲区, 然后
    //第三参数是采样点数, 然后第四参数是输出的缓冲区, 以及第五参数长度
    //QByteArray sendBuffer( (char*) stream,len); // 使用原缓冲区 避免拷贝
    QByteArray sendBuffer( (char*) opusData,nbBytes); //使用编码以后的缓冲区

    //编码并发送 注意是多线程. 此并非主线程.
    audio->slot_sendAudioFrame( sendBuffer );
}
