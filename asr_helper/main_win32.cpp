// sherpa_asr_helper - pure Win32 + Sherpa-ONNX C API (no Qt dependencies)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// #include <algorithm>  // unused
// #include <cmath>  // unused
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// Sherpa-ONNX C API struct definitions (consume side)
#include "c-api.h"

#pragma comment(lib, "ws2_32.lib")

// ===== Protocol constants (must match AsrClient) =====
namespace {
constexpr uint8_t  kMsgStatus = 100;
constexpr uint8_t  kMsgResult = 101;
constexpr uint8_t  kMsgAudio  = 2;
constexpr uint8_t  kMsgReset  = 3;
constexpr uint32_t kMaxFrameSize = 16 * 1024 * 1024;

// ===== QDataStream Qt_5_12 compatible serialization =====

void writeBE_uint32(uint8_t* dst, uint32_t v) {
    dst[0] = (v >> 24) & 0xFF; dst[1] = (v >> 16) & 0xFF;
    dst[2] = (v >> 8) & 0xFF;  dst[3] = v & 0xFF;
}

uint32_t readBE_uint32(const uint8_t* src) {
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) | ((uint32_t)src[2] << 8) | src[3];
}

void writeLE_uint32(uint8_t* dst, uint32_t v) {
    dst[0] = v & 0xFF; dst[1] = (v >> 8) & 0xFF;
    dst[2] = (v >> 16) & 0xFF; dst[3] = (v >> 24) & 0xFF;
}

uint32_t readLE_uint32(const uint8_t* src) {
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

void writeLE_uint64(uint8_t* dst, uint64_t v) {
    for (int i = 0; i < 8; i++) dst[i] = (v >> (i * 8)) & 0xFF;
}

uint64_t readLE_uint64(const uint8_t* src) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)src[i] << (i * 8));
    return v;
}

void writeBE_uint64(uint8_t* dst, uint64_t v) {
    for (int i = 0; i < 8; i++) dst[7 - i] = (v >> (i * 8)) & 0xFF;
}

uint64_t readBE_uint64(const uint8_t* src) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)src[7 - i] << (i * 8));
    return v;
}

// ===== Buffer helper (replaces QByteArray) =====
struct Buffer {
    std::vector<uint8_t> data;
    void append(const void* p, size_t n) {
        const uint8_t* b = static_cast<const uint8_t*>(p);
        data.insert(data.end(), b, b + n);
    }
    void consume(size_t n) { if (n <= data.size()) data.erase(data.begin(), data.begin() + n); }
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    const uint8_t* ptr() const { return data.data(); }
    void clear() { data.clear(); }
};

// ===== Serialization helpers =====
std::vector<uint8_t> ser_int32(int32_t v) {
    std::vector<uint8_t> b(4);
    writeBE_uint32(b.data(), static_cast<uint32_t>(v));
    return b;
}
std::vector<uint8_t> ser_int64(int64_t v) {
    std::vector<uint8_t> b(8);
    writeBE_uint64(b.data(), static_cast<uint64_t>(v));
    return b;
}
std::vector<uint8_t> ser_bool(bool v) {
    return { static_cast<uint8_t>(v ? 1 : 0) };
}
std::vector<uint8_t> ser_qstring(const std::string& utf8) {
    // QDataStream Qt_5_12 format: BE uint32 byte count + UTF-16BE bytes
    // NOTE: QDataStream default byte order is BigEndian. When reading QString,
    // it interprets raw bytes as UTF-16BE and converts to native (LE) via qFromBigEndian.
    // Sending UTF-16LE causes each ushort pair to be byte-swapped = garbled CJK.
    std::vector<uint8_t> r;

    if (utf8.empty()) {
        uint8_t zero[4] = {0, 0, 0, 0};
        r.insert(r.end(), zero, zero + 4);
        return r;
    }

    // Convert UTF-8 -> UTF-16BE via Win32 API
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::vector<WCHAR> wbuf(wlen);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), wbuf.data(), wlen);

    uint32_t byteCount = static_cast<uint32_t>(wlen * sizeof(WCHAR));
    uint8_t lenBuf[4];
    writeBE_uint32(lenBuf, byteCount);
    r.insert(r.end(), lenBuf, lenBuf + 4);
    for (int i = 0; i < wlen; i++) {
        r.push_back(static_cast<uint8_t>((wbuf[i] >> 8) & 0xFF)); // BE high byte
        r.push_back(static_cast<uint8_t>(wbuf[i] & 0xFF));        // BE low byte
    }
    return r;
}
}

// ===== Sherpa C API function pointer types =====
typedef const SherpaOnnxOnlineRecognizer* (*PFN_SherpaOnnxCreateOnlineRecognizer)(
    const SherpaOnnxOnlineRecognizerConfig* config);
typedef void (*PFN_SherpaOnnxDestroyOnlineRecognizer)(
    const SherpaOnnxOnlineRecognizer* recognizer);
typedef const SherpaOnnxOnlineStream* (*PFN_SherpaOnnxCreateOnlineStream)(
    const SherpaOnnxOnlineRecognizer* recognizer);
typedef void (*PFN_SherpaOnnxDestroyOnlineStream)(
    const SherpaOnnxOnlineStream* stream);
typedef void (*PFN_SherpaOnnxOnlineStreamAcceptWaveform)(
    const SherpaOnnxOnlineStream* stream, int32_t sample_rate,
    const float* samples, int32_t n);
typedef int32_t (*PFN_SherpaOnnxIsOnlineStreamReady)(
    const SherpaOnnxOnlineRecognizer* recognizer,
    const SherpaOnnxOnlineStream* stream);
typedef void (*PFN_SherpaOnnxDecodeOnlineStream)(
    const SherpaOnnxOnlineRecognizer* recognizer,
    const SherpaOnnxOnlineStream* stream);
typedef const SherpaOnnxOnlineRecognizerResult* (*PFN_SherpaOnnxGetOnlineStreamResult)(
    const SherpaOnnxOnlineRecognizer* recognizer,
    const SherpaOnnxOnlineStream* stream);
typedef void (*PFN_SherpaOnnxDestroyOnlineRecognizerResult)(
    const SherpaOnnxOnlineRecognizerResult* r);
typedef int32_t (*PFN_SherpaOnnxOnlineStreamIsEndpoint)(
    const SherpaOnnxOnlineRecognizer* recognizer,
    const SherpaOnnxOnlineStream* stream);
typedef void (*PFN_SherpaOnnxOnlineStreamReset)(
    const SherpaOnnxOnlineRecognizer* recognizer,
    const SherpaOnnxOnlineStream* stream);

// ===== SherpaEngine: DLL loading + recognizer lifecycle =====
class SherpaEngine {
public:
    ~SherpaEngine() { Shutdown(); }

    bool Init(const std::string& libDir, const std::string& modelDir, std::string* err) {
        // Build DLL path
        std::string dllPath = libDir + "\\sherpa-onnx-c-api.dll";
        dll_ = LoadLibraryA(dllPath.c_str());
        if (!dll_) {
            // Try without libDir prefix (already in PATH or exe dir)
            dll_ = LoadLibraryA("sherpa-onnx-c-api.dll");
        }
        if (!dll_) {
            if (err) *err = "Failed to load sherpa-onnx-c-api.dll (error " +
                            std::to_string(GetLastError()) + ")";
            return false;
        }

        // Resolve all function pointers
        #define RESOLVE(fn) \
            fn##_ = reinterpret_cast<PFN_##fn>(GetProcAddress(dll_, #fn)); \
            if (!fn##_) { \
                if (err) *err = "Failed to resolve " #fn; \
                return false; \
            }

        RESOLVE(SherpaOnnxCreateOnlineRecognizer);
        RESOLVE(SherpaOnnxDestroyOnlineRecognizer);
        RESOLVE(SherpaOnnxCreateOnlineStream);
        RESOLVE(SherpaOnnxDestroyOnlineStream);
        RESOLVE(SherpaOnnxOnlineStreamAcceptWaveform);
        RESOLVE(SherpaOnnxIsOnlineStreamReady);
        RESOLVE(SherpaOnnxDecodeOnlineStream);
        RESOLVE(SherpaOnnxGetOnlineStreamResult);
        RESOLVE(SherpaOnnxDestroyOnlineRecognizerResult);
        RESOLVE(SherpaOnnxOnlineStreamIsEndpoint);
        RESOLVE(SherpaOnnxOnlineStreamReset);

        #undef RESOLVE

        // Detect model files (prefer int8 quantized versions)
        std::string encPath = modelDir + "\\encoder.int8.onnx";
        std::string decPath = modelDir + "\\decoder.int8.onnx";
        std::string tokPath = modelDir + "\\tokens.txt";

        {
            std::ifstream test(encPath);
            if (!test) encPath = modelDir + "\\encoder.onnx";
        }
        {
            std::ifstream test(decPath);
            if (!test) decPath = modelDir + "\\decoder.onnx";
        }

        // Build config
        SherpaOnnxOnlineParaformerModelConfig paraformerCfg;
        memset(&paraformerCfg, 0, sizeof(paraformerCfg));
        paraformerCfg.encoder = encPath.c_str();
        paraformerCfg.decoder = decPath.c_str();

        SherpaOnnxOnlineModelConfig modelCfg;
        memset(&modelCfg, 0, sizeof(modelCfg));
        modelCfg.tokens = tokPath.c_str();
        modelCfg.paraformer = paraformerCfg;
        modelCfg.num_threads = 1;
        modelCfg.provider = "cpu";
        modelCfg.debug = 0;

        SherpaOnnxOnlineRecognizerConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.feat_config.sample_rate = 16000;
        cfg.feat_config.feature_dim = 80;
        cfg.model_config = modelCfg;
        cfg.decoding_method = "greedy_search";
        cfg.max_active_paths = 4;
        cfg.enable_endpoint = 1;
        cfg.rule1_min_trailing_silence = 3.0f;
        cfg.rule2_min_trailing_silence = 2.0f;
        cfg.rule3_min_utterance_length = 300.0f;

        recognizer_ = SherpaOnnxCreateOnlineRecognizer_(&cfg);
        if (!recognizer_) {
            if (err) *err = "SherpaOnnxCreateOnlineRecognizer returned NULL";
            return false;
        }

        fprintf(stderr, "[Sherpa] Engine initialized\n");
        fprintf(stderr, "[Sherpa]   encoder: %s\n", encPath.c_str());
        fprintf(stderr, "[Sherpa]   decoder: %s\n", decPath.c_str());
        fprintf(stderr, "[Sherpa]   tokens:  %s\n", tokPath.c_str());
        fflush(stderr);
        return true;
    }

    void Shutdown() {
        if (recognizer_ && SherpaOnnxDestroyOnlineRecognizer_) {
            SherpaOnnxDestroyOnlineRecognizer_(recognizer_);
            recognizer_ = nullptr;
        }
        if (dll_) {
            FreeLibrary(dll_);
            dll_ = nullptr;
        }
        // Null all fn pointers
        SherpaOnnxCreateOnlineRecognizer_ = nullptr;
        SherpaOnnxDestroyOnlineRecognizer_ = nullptr;
        SherpaOnnxCreateOnlineStream_ = nullptr;
        SherpaOnnxDestroyOnlineStream_ = nullptr;
        SherpaOnnxOnlineStreamAcceptWaveform_ = nullptr;
        SherpaOnnxIsOnlineStreamReady_ = nullptr;
        SherpaOnnxDecodeOnlineStream_ = nullptr;
        SherpaOnnxGetOnlineStreamResult_ = nullptr;
        SherpaOnnxDestroyOnlineRecognizerResult_ = nullptr;
        SherpaOnnxOnlineStreamIsEndpoint_ = nullptr;
        SherpaOnnxOnlineStreamReset_ = nullptr;
    }

    // Stream management
    const SherpaOnnxOnlineStream* CreateStream() {
        return SherpaOnnxCreateOnlineStream_(recognizer_);
    }
    void DestroyStream(const SherpaOnnxOnlineStream* s) {
        if (s) SherpaOnnxDestroyOnlineStream_(s);
    }

    // Inference wrappers
    void AcceptWaveform(const SherpaOnnxOnlineStream* s, int32_t sr,
                        const float* samples, int32_t n) {
        SherpaOnnxOnlineStreamAcceptWaveform_(s, sr, samples, n);
    }
    bool IsReady(const SherpaOnnxOnlineStream* s) const {
        return SherpaOnnxIsOnlineStreamReady_(recognizer_, s) != 0;
    }
    void Decode(const SherpaOnnxOnlineStream* s) {
        SherpaOnnxDecodeOnlineStream_(recognizer_, s);
    }
    const SherpaOnnxOnlineRecognizerResult* GetResult(
        const SherpaOnnxOnlineStream* s) {
        return SherpaOnnxGetOnlineStreamResult_(recognizer_, s);
    }
    void DestroyResult(const SherpaOnnxOnlineRecognizerResult* r) {
        if (r) SherpaOnnxDestroyOnlineRecognizerResult_(r);
    }
    bool IsEndpoint(const SherpaOnnxOnlineStream* s) const {
        return SherpaOnnxOnlineStreamIsEndpoint_(recognizer_, s) != 0;
    }
    void ResetStream(const SherpaOnnxOnlineStream* s) {
        SherpaOnnxOnlineStreamReset_(recognizer_, s);
    }

private:
    HMODULE dll_ = nullptr;
    const SherpaOnnxOnlineRecognizer* recognizer_ = nullptr;

    PFN_SherpaOnnxCreateOnlineRecognizer SherpaOnnxCreateOnlineRecognizer_ = nullptr;
    PFN_SherpaOnnxDestroyOnlineRecognizer SherpaOnnxDestroyOnlineRecognizer_ = nullptr;
    PFN_SherpaOnnxCreateOnlineStream SherpaOnnxCreateOnlineStream_ = nullptr;
    PFN_SherpaOnnxDestroyOnlineStream SherpaOnnxDestroyOnlineStream_ = nullptr;
    PFN_SherpaOnnxOnlineStreamAcceptWaveform SherpaOnnxOnlineStreamAcceptWaveform_ = nullptr;
    PFN_SherpaOnnxIsOnlineStreamReady SherpaOnnxIsOnlineStreamReady_ = nullptr;
    PFN_SherpaOnnxDecodeOnlineStream SherpaOnnxDecodeOnlineStream_ = nullptr;
    PFN_SherpaOnnxGetOnlineStreamResult SherpaOnnxGetOnlineStreamResult_ = nullptr;
    PFN_SherpaOnnxDestroyOnlineRecognizerResult SherpaOnnxDestroyOnlineRecognizerResult_ = nullptr;
    PFN_SherpaOnnxOnlineStreamIsEndpoint SherpaOnnxOnlineStreamIsEndpoint_ = nullptr;
    PFN_SherpaOnnxOnlineStreamReset SherpaOnnxOnlineStreamReset_ = nullptr;
};

// ===== AsrSession: wraps one Sherpa stream =====
class AsrSession {
public:
    struct DecodeResult { std::string text; bool isFinal; };

    explicit AsrSession(SherpaEngine* eng) : eng_(eng) {
        stream_ = eng_->CreateStream();
    }

    ~AsrSession() {
        if (stream_) eng_->DestroyStream(stream_);
    }

    void Accept(const int16_t* pcm, int samples) {
        if (!stream_) return;

        audioBuf_.insert(audioBuf_.end(), pcm, pcm + samples);

        std::vector<float> wave(samples);
        for (int i = 0; i < samples; i++)
            wave[i] = static_cast<float>(pcm[i]) / 32768.0f;  // normalize to [-1,1]; Sherpa multiplies by 32768 internally

        eng_->AcceptWaveform(stream_, 16000, wave.data(), samples);

        // Drain ready frames
        while (eng_->IsReady(stream_)) {
            eng_->Decode(stream_);
        }
    }

    DecodeResult Decode() {
        if (!stream_) return {"", false};

        const SherpaOnnxOnlineRecognizerResult* r = eng_->GetResult(stream_);
        std::string text;
        if (r && r->text)
            text = std::string(r->text);
        fprintf(stderr, "[Sherpa raw] %s\n", text.c_str()); fflush(stderr);
        eng_->DestroyResult(r);

        std::string result;
        if (!text.empty() && text != lastText_) {
            result = text;
            lastText_ = text;
        }

        bool isFinal = eng_->IsEndpoint(stream_);
        if (isFinal) {
            // Full-segment re-decode for accuracy on endpoint
            if (!audioBuf_.empty()) {
                fprintf(stderr, "[Sherpa] ENDPOINT: full re-decode (%zu samples)\n", audioBuf_.size()); fflush(stderr);
                auto* fullStream = eng_->CreateStream();
                // Feed in 500ms chunks like streaming, not all at once
                const int chunkSize = 8000;  // 500ms @ 16kHz
                for (size_t pos = 0; pos < audioBuf_.size(); pos += chunkSize) {
                    int n = (int)std::min((size_t)chunkSize, audioBuf_.size() - pos);
                    std::vector<float> wave(n);
                    for (int i = 0; i < n; i++)
                        wave[i] = static_cast<float>(audioBuf_[pos + i]) / 32768.0f;
                    eng_->AcceptWaveform(fullStream, 16000, wave.data(), n);
                    while (eng_->IsReady(fullStream))
                        eng_->Decode(fullStream);
                }
                auto* fullR = eng_->GetResult(fullStream);
                if (fullR && fullR->text) {
                    result = std::string(fullR->text);
                    fprintf(stderr, "[Sherpa] Full re-decode result: %s\n", fullR->text); fflush(stderr);
                }
                eng_->DestroyResult(fullR);
                eng_->DestroyStream(fullStream);
            }

            audioBuf_.clear();
            eng_->ResetStream(stream_);
            lastText_.clear();
        }

        return {result, isFinal};
    }

    void Reset() {
        if (stream_) {
            eng_->ResetStream(stream_);
            audioBuf_.clear();
            lastText_.clear();
        }
    }

private:
    std::vector<int16_t> audioBuf_;
    SherpaEngine* eng_;
    const SherpaOnnxOnlineStream* stream_ = nullptr;
    std::string lastText_;
};

// ===== Winsock2 TCP Server =====
class AsrServer {
public:
    AsrServer(uint16_t port, SherpaEngine* eng) : port_(port), eng_(eng) {}

    bool Start(std::string* err) {
        listenSock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSock_ == INVALID_SOCKET) { if (err) *err = "socket() failed"; return false; }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port_);
        if (bind(listenSock_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            if (err) *err = "bind() failed"; closesocket(listenSock_); return false;
        }
        if (listen(listenSock_, 1) == SOCKET_ERROR) {
            if (err) *err = "listen() failed"; closesocket(listenSock_); return false;
        }
        fprintf(stderr, "[Sherpa] Listening on port %u\n", port_); fflush(stderr);
        return true;
    }

    void Run() {
        running_ = true;
        while (running_) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(listenSock_, &fds);
            SOCKET maxFd = listenSock_;
            if (clientSock_ != INVALID_SOCKET) {
                FD_SET(clientSock_, &fds);
                if (clientSock_ > maxFd) maxFd = clientSock_;
            }
            timeval tv{1, 0};
            int ret = select((int)maxFd + 1, &fds, nullptr, nullptr, &tv);
            if (ret <= 0) continue;
            if (FD_ISSET(listenSock_, &fds)) OnAccept();
            if (clientSock_ != INVALID_SOCKET && FD_ISSET(clientSock_, &fds)) OnRead();
        }
    }

    void Stop() { running_ = false; }

private:
    void OnAccept() {
        if (clientSock_ != INVALID_SOCKET) {
            SOCKET tmp = accept(listenSock_, nullptr, nullptr);
            closesocket(tmp);
            return;
        }
        clientSock_ = accept(listenSock_, nullptr, nullptr);
        if (clientSock_ == INVALID_SOCKET) return;
        fprintf(stderr, "[Sherpa] Client connected\n"); fflush(stderr);
        SendStatus(true, "ready");
    }

    void OnRead() {
        uint8_t tmp[65536];
        int n = recv(clientSock_, (char*)tmp, sizeof(tmp), 0);
        if (n <= 0) { OnDisconnect(); return; }
        buf_.append(tmp, n);
        Parse();
    }

    void OnDisconnect() {
        closesocket(clientSock_);
        clientSock_ = INVALID_SOCKET;
        buf_.clear();
        sess_.reset();
        fprintf(stderr, "[Sherpa] Client disconnected\n"); fflush(stderr);
    }

    void Parse() {
        while (buf_.size() >= 4) {
            uint32_t sz = readLE_uint32(buf_.ptr());
            if (sz == 0 || sz > kMaxFrameSize) {
                buf_.clear();
                SendStatus(false, "bad frame");
                return;
            }
            if (buf_.size() < 4 + sz) return;
            uint8_t tp = buf_.ptr()[4];
            const uint8_t* pl = buf_.ptr() + 5;
            size_t plLen = sz - 1;

            if (tp == kMsgAudio) {
                if (plLen < 16) { buf_.consume(4 + sz); continue; }
                int32_t uid = (int32_t)readBE_uint32(pl);
                int64_t ts = (int64_t)readBE_uint64(pl + 4);
                uint32_t pcmLen = readBE_uint32(pl + 12);
                fprintf(stderr, "[Sherpa] Audio: uid=%d ts=%lld pcmLen=%u\n", uid, ts, pcmLen); fflush(stderr);
                fprintf(stderr, "[Sherpa] Audio: uid=%d ts=%lld pcmLen=%u\n", uid, ts, pcmLen); fflush(stderr);
                if (12 + 4 + pcmLen > plLen) { buf_.consume(4 + sz); continue; }
                const int16_t* pcm = reinterpret_cast<const int16_t*>(pl + 16);
                int samples = (int)pcmLen / (int)sizeof(int16_t);

                if (!sess_) sess_ = std::make_unique<AsrSession>(eng_);
                sess_->Accept(pcm, samples);
                auto [tex, fin] = sess_->Decode();
                if (!tex.empty()) SendResult(uid, tex, fin, ts);
            } else if (tp == kMsgReset) {
                if (sess_) sess_->Reset();
            }
            buf_.consume(4 + sz);
        }
    }

    void SendFrame(uint8_t tp, const std::vector<uint8_t>& pl) {
        if (clientSock_ == INVALID_SOCKET) return;
        uint32_t sz = 1 + (uint32_t)pl.size();
        uint8_t hdr[5];
        writeLE_uint32(hdr, sz);
        hdr[4] = tp;
        send(clientSock_, (const char*)hdr, 5, 0);
        if (!pl.empty()) send(clientSock_, (const char*)pl.data(), (int)pl.size(), 0);
    }

    void SendStatus(bool ok, const std::string& msg) {
        std::vector<uint8_t> pl = ser_bool(ok);
        auto qstr = ser_qstring(msg);
        pl.insert(pl.end(), qstr.begin(), qstr.end());
        SendFrame(kMsgStatus, pl);
    }

    void SendResult(int uid, const std::string& tex, bool fin, int64_t ts) {
        std::vector<uint8_t> pl = ser_int32(uid);
        auto b = ser_bool(fin); pl.insert(pl.end(), b.begin(), b.end());
        auto i64 = ser_int64(ts); pl.insert(pl.end(), i64.begin(), i64.end());
        auto qstr = ser_qstring(tex);
        pl.insert(pl.end(), qstr.begin(), qstr.end());
        // Debug: dump raw payload bytes
        fprintf(stderr, "[Sherpa] SendResult: uid=%d fin=%d ts=%lld text=[%s]\n", uid, fin, ts, tex.c_str());
        fprintf(stderr, "[Sherpa]   payload(%zuB): ", pl.size());
        for (size_t i = 0; i < pl.size() && i < 64; i++)
            fprintf(stderr, "%02x", pl[i]);
        fprintf(stderr, "\n"); fflush(stderr);
        SendFrame(kMsgResult, pl);
    }

    SOCKET listenSock_ = INVALID_SOCKET;
    SOCKET clientSock_ = INVALID_SOCKET;
    Buffer buf_;
    uint16_t port_;
    SherpaEngine* eng_;
    std::unique_ptr<AsrSession> sess_;
    bool running_ = false;
};

// ===== Main =====
int main(int argc, char* argv[]) {
    uint16_t port = 34981;
    std::string modelDir;
    std::string libDir;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            port = (uint16_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--model-dir") == 0 && i + 1 < argc)
            modelDir = argv[++i];
        else if (strcmp(argv[i], "--lib-dir") == 0 && i + 1 < argc)
            libDir = argv[++i];
    }

    // Default paths relative to exe
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string exeDir(exePath);
    exeDir = exeDir.substr(0, exeDir.rfind('\\'));

    if (libDir.empty())
        libDir = exeDir;  // DLLs alongside exe

    if (modelDir.empty()) {
        // Try subdir "model" first, then hardcoded WeChat path
        modelDir = exeDir + "\\model";
        std::ifstream test(modelDir + "\\tokens.txt");
        if (!test) {
            modelDir = "E:\\colinCode\\stage_project\\Wechat\\sherpa-onnx-streaming-paraformer-bilingual-zh-en";
        }
    }

    fprintf(stderr, "=== Sherpa ASR Helper (Win32) ===\n");
    fprintf(stderr, "DLL dir: %s\n", libDir.c_str());
    fprintf(stderr, "Model:   %s\n", modelDir.c_str());
    fprintf(stderr, "Port:    %u\n", port);
    fflush(stderr);

    // Add libDir to DLL search path
    SetDllDirectoryA(libDir.c_str());

    // Redirect stderr to log file (WeChat launches us without console)
    std::string logPath = exeDir + "\\sherpa_asr.log";
    freopen(logPath.c_str(), "a", stderr);
    setvbuf(stderr, nullptr, _IONBF, 0);  // unbuffered for real-time logging

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SherpaEngine eng;
    std::string err;
    if (!eng.Init(libDir, modelDir, &err)) {
        fprintf(stderr, "ASR init failed: %s\n", err.c_str());
        WSACleanup();
        return 2;
    }

    AsrServer srv(port, &eng);
    if (!srv.Start(&err)) {
        fprintf(stderr, "Server start failed: %s\n", err.c_str());
        WSACleanup();
        return 3;
    }

    fprintf(stderr, "[Sherpa] Server running, waiting for connections...\n");
    fflush(stderr);
    srv.Run();

    WSACleanup();
    return 0;
}
