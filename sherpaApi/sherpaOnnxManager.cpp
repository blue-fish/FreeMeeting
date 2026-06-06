#include "sherpaOnnxManager.h"
#include <QDebug>
#include <QCoreApplication>
#include <cstring>

SherpaOnnxManager& SherpaOnnxManager::instance()
{
    static SherpaOnnxManager manager;
    return manager;
}

SherpaOnnxManager::SherpaOnnxManager(QObject* parent)
    : QObject(parent)
    , m_recognizer(nullptr, nullptr)
    , m_stream(nullptr, nullptr)
{
}

SherpaOnnxManager::~SherpaOnnxManager()
{
    shutdown();
}

bool SherpaOnnxManager::initialize(const QString& tokensPath,
                                   const QString& encoderPath,
                                   const QString& decoderPath,
                                   //const QString& joinerPath,
                                   int numThreads,
                                   const QString& provider)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        qWarning() << "SherpaOnnxManager already initialized";
        return true;
    }

    if (!loadLibrary()) {
        qCritical() << "Failed to load sherpa-onnx library";
        return false;
    }

    // 配置识别器
    SherpaOnnxOnlineRecognizerConfig config = {0};
    // 关键修改：用 paraformer 字段，而不是 transducer
    QByteArray tokensBytes  = tokensPath.toUtf8();
    QByteArray encoderBytes = encoderPath.toUtf8();
    QByteArray decoderBytes = decoderPath.toUtf8();
    QByteArray providerBytes = provider.toUtf8();

    config.model_config.tokens = tokensBytes.constData();
    config.model_config.paraformer.encoder = encoderBytes.constData();  // ← 改这里
    config.model_config.paraformer.decoder = decoderBytes.constData();
    config.model_config.num_threads = numThreads;
    config.model_config.provider = providerBytes.constData();
    config.model_config.debug = 0;
    config.model_config.model_type="paraformer";

    config.decoding_method = "greedy_search";
    config.max_active_paths = 4;
    config.feat_config.sample_rate = 16000;
    config.feat_config.feature_dim = 80;
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4;
    config.rule2_min_trailing_silence = 1.2;
    config.rule3_min_utterance_length = 300;

    // 创建 recognizer（使用原始指针，然后交给 unique_ptr）
    SherpaOnnxOnlineRecognizer* rawRecognizer = m_createRecognizer(&config);
    if (!rawRecognizer) {
        qCritical() << "Failed to create online recognizer";
        cleanup();
        return false;
    }

    // 自定义删除器
    auto recognizerDeleter = [this](SherpaOnnxOnlineRecognizer* p) {
        if (p && m_destroyRecognizer) {
            m_destroyRecognizer(p);
            qDebug() << "Recognizer destroyed";
        }
    };
    m_recognizer = std::unique_ptr<SherpaOnnxOnlineRecognizer, RecognizerDeleter>(
        rawRecognizer, recognizerDeleter);

    // 创建 stream
    SherpaOnnxOnlineStream* rawStream = m_createStream(m_recognizer.get());
    if (!rawStream) {
        qCritical() << "Failed to create online stream";
        m_recognizer.reset();  // 自动调用删除器释放 recognizer
        cleanup();
        return false;
    }

    auto streamDeleter = [this](SherpaOnnxOnlineStream* p) {
        if (p && m_destroyStream) {
            m_destroyStream(p);
            qDebug() << "Stream destroyed";
        }
    };
    m_stream = std::unique_ptr<SherpaOnnxOnlineStream, StreamDeleter>(
        rawStream, streamDeleter);

    m_initialized = true;
    qDebug() << "SherpaOnnxManager initialized successfully";
    return true;
}

void SherpaOnnxManager::acceptWaveform(const float* samples, int32_t n)
{
    if (!m_initialized) return;
    QMutexLocker locker(&m_mutex);
    if (!m_recognizer || !m_stream) return;

    m_acceptWaveform(m_stream.get(), 16000, samples, n);

    // 每次喂完音频立即尝试解码并获取结果
    decodeAndNotify();
}

void SherpaOnnxManager::decode()
{
    if (!m_initialized) return;
    QMutexLocker locker(&m_mutex);
    if (!m_recognizer || !m_stream) return;

    while (m_isReady(m_recognizer.get(), m_stream.get())) {
        m_decode(m_recognizer.get(), m_stream.get());
    }
}

QString SherpaOnnxManager::getResult()
{
    if (!m_initialized) return QString();
    QMutexLocker locker(&m_mutex);
    if (!m_recognizer || !m_stream) return QString();

    SherpaOnnxOnlineRecognizerResult* res = m_getResult(m_recognizer.get(), m_stream.get());
    if (!res) return QString();

    QString text = QString::fromUtf8(res->text);
    m_destroyResult(res);
    return text;
}

void SherpaOnnxManager::resetStream()
{
    if (!m_initialized) return;
    QMutexLocker locker(&m_mutex);
    if (!m_recognizer || !m_stream) return;
    m_resetStream(m_recognizer.get(), m_stream.get());
}

void SherpaOnnxManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return;

    // 先释放 stream，再释放 recognizer
    m_stream.reset();    // 自动调用删除器
    m_recognizer.reset();

    cleanup();
    m_initialized = false;
    qDebug() << "SherpaOnnxManager shutdown";
}

bool SherpaOnnxManager::loadLibrary()
{
    // 根据平台设置动态库名称
#ifdef Q_OS_WIN
    m_lib.setFileName("sherpa-onnx-c-api");
#else
    m_lib.setFileName("libsherpa-onnx.so");
#endif

    if (!m_lib.load()) {
        qCritical() << "Load sherpa-onnx failed:" << m_lib.errorString();
        return false;
    }

    // 解析所有需要的函数指针
    m_createRecognizer = (CreateRecognizerFunc)m_lib.resolve("SherpaOnnxCreateOnlineRecognizer");
    m_destroyRecognizer = (DestroyRecognizerFunc)m_lib.resolve("SherpaOnnxDestroyOnlineRecognizer");
    m_createStream = (CreateStreamFunc)m_lib.resolve("SherpaOnnxCreateOnlineStream");
    m_destroyStream = (DestroyStreamFunc)m_lib.resolve("SherpaOnnxDestroyOnlineStream");
    m_acceptWaveform = (AcceptWaveformFunc)m_lib.resolve("SherpaOnnxOnlineStreamAcceptWaveform");
    m_decode = (DecodeFunc)m_lib.resolve("SherpaOnnxDecodeOnlineStream");
    m_getResult = (GetResultFunc)m_lib.resolve("SherpaOnnxGetOnlineStreamResult");
    m_destroyResult = (DestroyResultFunc)m_lib.resolve("SherpaOnnxDestroyOnlineRecognizerResult");
    m_isReady = (IsReadyFunc)m_lib.resolve("SherpaOnnxIsOnlineStreamReady");
    m_isEndpoint = (IsEndpointFunc)m_lib.resolve("SherpaOnnxOnlineStreamIsEndpoint");
    m_resetStream = (ResetStreamFunc)m_lib.resolve("SherpaOnnxOnlineStreamReset");

    if (!m_createRecognizer || !m_destroyRecognizer || !m_createStream ||
        !m_destroyStream || !m_acceptWaveform || !m_decode || !m_getResult ||
        !m_destroyResult || !m_isReady || !m_isEndpoint || !m_resetStream) {
        qCritical() << "Failed to resolve sherpa-onnx functions";
        return false;
    }
    return true;
}

void SherpaOnnxManager::cleanup()
{
    m_lib.unload();
    // 函数指针被清空，但 unique_ptr 已经释放了资源，此处只需清零指针即可
    m_createRecognizer = nullptr;
    m_destroyRecognizer = nullptr;
    m_createStream = nullptr;
    m_destroyStream = nullptr;
    m_acceptWaveform = nullptr;
    m_decode = nullptr;
    m_getResult = nullptr;
    m_destroyResult = nullptr;
    m_isReady = nullptr;
    m_isEndpoint = nullptr;
    m_resetStream = nullptr;
}

void SherpaOnnxManager::decodeAndNotify()
{
    // 解码
    while (m_isReady(m_recognizer.get(), m_stream.get())) {
        m_decode(m_recognizer.get(), m_stream.get());
    }

    // 获取结果
    SherpaOnnxOnlineRecognizerResult* res = m_getResult(m_recognizer.get(), m_stream.get());
    if (res && res->text && strlen(res->text) > 0) {
        QString text = QString::fromUtf8(res->text).toLower();
        emit textRecognized(text);
    }
    if (res) m_destroyResult(res);

    // 检测端点，如果端点触发则自动重置流（开始下一句）
    if (m_isEndpoint(m_recognizer.get(), m_stream.get())) {
        m_resetStream(m_recognizer.get(), m_stream.get());
    }
}
