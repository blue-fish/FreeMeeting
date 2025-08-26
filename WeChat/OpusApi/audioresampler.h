#ifndef AUDIORESAMPLER_H
#define AUDIORESAMPLER_H

#include <QByteArray>
#include <vector>

class AudioResampler
{
public:
    // 简单的降采样：48kHz -> 16kHz (每3个样本取1个)
    static QByteArray resample48to16(const QByteArray& input) {
        const int16_t* inputSamples = reinterpret_cast<const int16_t*>(input.data());
        int inputSampleCount = input.size() / sizeof(int16_t);

        std::vector<int16_t> outputSamples;
        outputSamples.reserve(inputSampleCount / 3);

        // 使用简单的低通滤波器 + 降采样
        for (int i = 0; i < inputSampleCount - 6; i += 3) {
            // 简单的3点平均滤波
            int32_t sample = (inputSamples[i] + inputSamples[i+1] + inputSamples[i+2]) / 3;
            outputSamples.push_back(static_cast<int16_t>(sample));
        }

        return QByteArray(reinterpret_cast<const char*>(outputSamples.data()),
                         outputSamples.size() * sizeof(int16_t));
    }
};

#endif // AUDIORESAMPLER_H
