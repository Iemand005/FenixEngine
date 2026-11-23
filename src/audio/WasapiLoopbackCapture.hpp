#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#pragma comment(lib, "ole32.lib")

// Locally defined KSDATAFORMAT subtypes so we don't have to pull in <ksmedia.h>.
static const GUID FE_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =
    { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID FE_KSDATAFORMAT_SUBTYPE_PCM =
    { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

class WasapiLoopbackCapture {
public:
    bool Init() {
        if (captureClient) return true;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        // RPC_E_CHANGED_MODE just means COM is already up on this thread; fine.
        comInitialized = SUCCEEDED(hr);

        IMMDeviceEnumerator* enumerator = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
        if (FAILED(hr) || !enumerator) {
            std::cout << "WASAPI: CoCreateInstance(MMDeviceEnumerator) failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            return false;
        }

        IMMDevice* device = nullptr;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        enumerator->Release();
        if (FAILED(hr) || !device) {
            std::cout << "WASAPI: no default render endpoint: 0x"
                      << std::hex << hr << std::dec << "\n";
            return false;
        }

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
        device->Release();
        if (FAILED(hr) || !audioClient) {
            std::cout << "WASAPI: Activate(IAudioClient) failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            return false;
        }

        WAVEFORMATEX* mixFormat = nullptr;
        hr = audioClient->GetMixFormat(&mixFormat);
        if (FAILED(hr) || !mixFormat) {
            std::cout << "WASAPI: GetMixFormat failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            return false;
        }

        channels = mixFormat->nChannels;
        sampleRate = mixFormat->nSamplesPerSec;
        format = DetectFormat(mixFormat);

        // 1 second shared-mode buffer; loopback needs a render endpoint in the
        // shared engine, which is exactly what we activated above.
        hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                     10000000, 0, mixFormat, nullptr);
        CoTaskMemFree(mixFormat);

        if (FAILED(hr)) {
            std::cout << "WASAPI loopback Initialize failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            audioClient->Release();
            audioClient = nullptr;
            return false;
        }

        hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
        if (FAILED(hr) || !captureClient) {
            std::cout << "WASAPI: GetService(IAudioCaptureClient) failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            audioClient->Release();
            audioClient = nullptr;
            return false;
        }

        hr = audioClient->Start();
        if (FAILED(hr)) {
            std::cout << "WASAPI: IAudioClient::Start failed: 0x"
                      << std::hex << hr << std::dec << "\n";
            return false;
        }

        return true;
    }

    void Poll(std::vector<float>& out) {
        if (!captureClient) return;

        UINT32 packetLength = 0;
        if (FAILED(captureClient->GetNextPacketSize(&packetLength))) return;

        while (packetLength != 0) {
            BYTE* data = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            HRESULT hr = captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

            if (silent || !data) {
                out.insert(out.end(), numFrames, 0.0f);
            } else {
                const UINT32 channelCount = channels ? channels : 1;
                for (UINT32 i = 0; i < numFrames; ++i) {
                    float mono = 0.0f;
                    for (UINT32 c = 0; c < channelCount; ++c)
                        mono += SampleToFloat(data, i * channelCount + c);
                    out.push_back(mono / (float)channelCount);
                }
            }

            captureClient->ReleaseBuffer(numFrames);

            if (FAILED(captureClient->GetNextPacketSize(&packetLength))) break;
        }
    }

    void Shutdown() {
        if (audioClient) audioClient->Stop();
        if (captureClient) { captureClient->Release(); captureClient = nullptr; }
        if (audioClient) { audioClient->Release(); audioClient = nullptr; }
        if (comInitialized) { CoUninitialize(); comInitialized = false; }
    }

    bool IsCapturing() const { return captureClient != nullptr; }

    ~WasapiLoopbackCapture() { Shutdown(); }

private:
    enum class SampleFormat { Float32, PCM16, PCM32, PCM8, Unknown };

    static SampleFormat DetectFormat(const WAVEFORMATEX* fmt) {
        if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            return SampleFormat::Float32;

        if (fmt->wFormatTag == WAVE_FORMAT_PCM) {
            switch (fmt->wBitsPerSample) {
                case 16: return SampleFormat::PCM16;
                case 32: return SampleFormat::PCM32;
                case 8:  return SampleFormat::PCM8;
                default: return SampleFormat::Unknown;
            }
        }

        if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE && fmt->cbSize >= 22) {
            const WAVEFORMATEXTENSIBLE* ext = (const WAVEFORMATEXTENSIBLE*)fmt;
            if (IsEqualGUID(ext->SubFormat, FE_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
                return SampleFormat::Float32;
            if (IsEqualGUID(ext->SubFormat, FE_KSDATAFORMAT_SUBTYPE_PCM)) {
                switch (fmt->wBitsPerSample) {
                    case 16: return SampleFormat::PCM16;
                    case 32: return SampleFormat::PCM32;
                    case 8:  return SampleFormat::PCM8;
                    default: return SampleFormat::Unknown;
                }
            }
        }

        return SampleFormat::Unknown;
    }

    float SampleToFloat(const BYTE* base, UINT32 index) const {
        switch (format) {
            case SampleFormat::Float32: return ((const float*)base)[index];
            case SampleFormat::PCM16:   return ((const int16_t*)base)[index] / 32768.0f;
            case SampleFormat::PCM32:   return ((const int32_t*)base)[index] / 2147483648.0f;
            case SampleFormat::PCM8:    return (((const uint8_t*)base)[index] - 128) / 128.0f;
            default:                    return 0.0f;
        }
    }

    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    UINT32 channels = 2;
    UINT32 sampleRate = 48000;
    SampleFormat format = SampleFormat::Float32;
    bool comInitialized = false;
};

inline WasapiLoopbackCapture g_loopback;
