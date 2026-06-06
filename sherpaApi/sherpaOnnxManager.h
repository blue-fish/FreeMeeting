#ifndef SHERPA_ONNX_MANAGER_H
#define SHERPA_ONNX_MANAGER_H

#include <QObject>
#include <QMutex>
#include <QLibrary>
#include <memory>
#include <functional>
#include "c-api.h"   // 您的路径，或者 <sherpa-onnx/c-api/c-api.h>

class SherpaOnnxManager : public QObject
{
    Q_OBJECT
public:
    // 获取单例实例（线程安全）
    static SherpaOnnxManager& instance();

    // 初始化识别器（模型文件路径请根据实际情况修改）
    bool initialize(const QString& tokensPath,
                    const QString& encoderPath,
                    const QString& decoderPath,
                    //const QString& joinerPath,
                    int numThreads = 1,
                    const QString& provider = "cpu");

    // 是否已成功初始化
    bool isInitialized() const { return m_initialized; }

    // 喂入 16kHz 的浮点 PCM 数据（范围 [-1, 1]）
    void acceptWaveform(const float* samples, int32_t n);

    // 手动触发解码（通常不需要单独调用，acceptWaveform 内部已做）
    void decode();

    // 获取当前识别出的文本（非阻塞，若无新文本返回空字符串）
    QString getResult();

    // 重置当前流（用于端点检测后开始新句子）
    void resetStream();

    // 关闭识别器，释放资源
    void shutdown();

signals:
    // 当识别出新的文本时发射（主线程安全）
    void textRecognized(const QString& text);

private:
    SherpaOnnxManager(QObject* parent = nullptr);
    ~SherpaOnnxManager();

    // 禁止拷贝和赋值
    SherpaOnnxManager(const SherpaOnnxManager&) = delete;
    SherpaOnnxManager& operator=(const SherpaOnnxManager&) = delete;

    // 加载动态库并解析函数指针
    bool loadLibrary();

    // 清理动态库相关资源
    void cleanup();

    // 内部方法：执行一次解码并发射结果
    void decodeAndNotify();

private:
    QLibrary m_lib;
    bool m_initialized = false;
    mutable QMutex m_mutex;

    // 自定义删除器类型
    using RecognizerDeleter = std::function<void(SherpaOnnxOnlineRecognizer*)>;
    using StreamDeleter = std::function<void(SherpaOnnxOnlineStream*)>;

    // 资源对象：使用 unique_ptr + 自定义删除器实现 RAII
    std::unique_ptr<SherpaOnnxOnlineRecognizer, RecognizerDeleter> m_recognizer;
    std::unique_ptr<SherpaOnnxOnlineStream, StreamDeleter> m_stream;

    // 函数指针类型定义
    typedef SherpaOnnxOnlineRecognizer* (*CreateRecognizerFunc)(const SherpaOnnxOnlineRecognizerConfig*);
    typedef void (*DestroyRecognizerFunc)(SherpaOnnxOnlineRecognizer*);
    typedef SherpaOnnxOnlineStream* (*CreateStreamFunc)(SherpaOnnxOnlineRecognizer*);
    typedef void (*DestroyStreamFunc)(SherpaOnnxOnlineStream*);
    typedef void (*AcceptWaveformFunc)(SherpaOnnxOnlineStream*, int32_t, const float*, int32_t);
    typedef void (*DecodeFunc)(SherpaOnnxOnlineRecognizer*, SherpaOnnxOnlineStream*);
    typedef SherpaOnnxOnlineRecognizerResult* (*GetResultFunc)(SherpaOnnxOnlineRecognizer*, SherpaOnnxOnlineStream*);
    typedef void (*DestroyResultFunc)(SherpaOnnxOnlineRecognizerResult*);
    typedef int32_t (*IsReadyFunc)(SherpaOnnxOnlineRecognizer*, SherpaOnnxOnlineStream*);
    typedef int32_t (*IsEndpointFunc)(SherpaOnnxOnlineRecognizer*, SherpaOnnxOnlineStream*);
    typedef void (*ResetStreamFunc)(SherpaOnnxOnlineRecognizer*, SherpaOnnxOnlineStream*);

    // 函数指针成员（普通指针，不需要智能指针）
    CreateRecognizerFunc   m_createRecognizer = nullptr;
    DestroyRecognizerFunc  m_destroyRecognizer = nullptr;
    CreateStreamFunc       m_createStream = nullptr;
    DestroyStreamFunc      m_destroyStream = nullptr;
    AcceptWaveformFunc     m_acceptWaveform = nullptr;
    DecodeFunc             m_decode = nullptr;
    GetResultFunc          m_getResult = nullptr;
    DestroyResultFunc      m_destroyResult = nullptr;
    IsReadyFunc            m_isReady = nullptr;
    IsEndpointFunc         m_isEndpoint = nullptr;
    ResetStreamFunc        m_resetStream = nullptr;
};

#endif // SHERPA_ONNX_MANAGER_H
