// #inclde 
#include <iostream>
#include <vector>
#include <mmdeviceapi.h>
#include <audioclient.h>
#pragma comment(lib, "ole32.lib")

class WasapiLoopbackCapture {
public:
    bool Init() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        IMMDeviceEnumerator* enumerator = nullptr;
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);

        IMMDevice* device = nullptr;
        enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device); // render device, in loopback mode

        device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);

        WAVEFORMATEX* mixFormat = nullptr;
        audioClient->GetMixFormat(&mixFormat);
        channels = mixFormat->nChannels;

        HRESULT hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, mixFormat, nullptr);

        CoTaskMemFree(mixFormat);
        device->Release();
        enumerator->Release();

        if (FAILED(hr)) {
            std::cout << "WASAPI loopback Initialize failed: " << std::hex << hr << "\n";
            return false;
        }

        audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
        audioClient->Start();
        return true;
    }

    void Poll(std::vector<float>& out) {
        if (!captureClient) return;

        UINT32 packetLength = 0;
        captureClient->GetNextPacketSize(&packetLength);

        while (packetLength != 0) {
            BYTE* data;
            UINT32 numFrames;
            DWORD flags;
            captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);

            bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            float* samples = (float*)data;

            for (UINT32 i = 0; i < numFrames; ++i) {
                float mono = 0.0f;
                if (!silent) {
                    for (UINT32 c = 0; c < channels; ++c)
                        mono += samples[i * channels + c];
                    mono /= (float)channels;
                }
                out.push_back(mono);
            }

            captureClient->ReleaseBuffer(numFrames);
            captureClient->GetNextPacketSize(&packetLength);
        }
    }

    ~WasapiLoopbackCapture() {
        if (audioClient) audioClient->Stop();
        if (captureClient) captureClient->Release();
        if (audioClient) audioClient->Release();
    }

private:
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    UINT32 channels = 2;
};

WasapiLoopbackCapture g_loopback;