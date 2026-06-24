#pragma once

// Linux equivalent of WasapiLoopbackCapture: captures whatever is currently
// playing through the system's default output device.
//
// How it works: we open a normal *capture* stream (PW_DIRECTION_INPUT), but
// tag it with PW_KEY_STREAM_CAPTURE_SINK = "true". This tells the session
// manager (WirePlumber) to link our stream to the MONITOR ports of the
// default sink instead of a microphone. Combined with
// PW_STREAM_FLAG_AUTOCONNECT, it will also automatically re-link if the user
// changes their default output device later -- no polling/rebuilding needed
// on our end.
//
// Build requirements: libpipewire-0.3 (dev headers), e.g.
//   Debian/Ubuntu: sudo apt install libpipewire-0.3-dev
//   Fedora:        sudo dnf install pipewire-devel
//   Arch:          sudo pacman -S libpipewire
//
// CMake:
//   find_package(PkgConfig REQUIRED)
//   pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
//   target_include_directories(<target> PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
//   target_link_libraries(<target> PRIVATE ${PIPEWIRE_LIBRARIES})
//   target_compile_options(<target> PRIVATE ${PIPEWIRE_CFLAGS_OTHER})

#include <vector>
#include <mutex>
#include <iostream>
#include <cstdint>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

class PipeWireLoopbackCapture {
public:
    bool Init(int sampleRate = 48000, int channels = 2) {
        m_channels = channels;

        pw_init(nullptr, nullptr);

        m_loop = pw_thread_loop_new("pw-loopback-capture", nullptr);
        if (!m_loop) {
            std::cout << "PipeWireLoopbackCapture: failed to create thread loop\n";
            return false;
        }

        pw_thread_loop_lock(m_loop);

        m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
        if (!m_context) {
            std::cout << "PipeWireLoopbackCapture: failed to create context\n";
            pw_thread_loop_unlock(m_loop);
            return false;
        }

        m_core = pw_context_connect(m_context, nullptr, 0);
        if (!m_core) {
            std::cout << "PipeWireLoopbackCapture: failed to connect to pipewire\n";
            pw_thread_loop_unlock(m_loop);
            return false;
        }

        pw_properties* props = pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "Music",
            PW_KEY_STREAM_CAPTURE_SINK, "true",   // <-- the loopback trick
            PW_KEY_NODE_NAME, "loopback-capture",
            nullptr);

        m_stream = pw_stream_new(m_core, "loopback-capture", props);
        if (!m_stream) {
            std::cout << "PipeWireLoopbackCapture: failed to create stream\n";
            pw_thread_loop_unlock(m_loop);
            return false;
        }

        static pw_stream_events streamEvents{};
        streamEvents.version = PW_VERSION_STREAM_EVENTS;
        streamEvents.process = OnProcess;
        streamEvents.state_changed = OnStateChanged;

        pw_stream_add_listener(m_stream, &m_streamListener, &streamEvents, this);

        uint8_t buffer[1024];
        spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        spa_audio_info_raw info{};
        info.format   = SPA_AUDIO_FORMAT_F32;
        info.rate     = (uint32_t)sampleRate;
        info.channels = (uint32_t)channels;
        if (channels == 1) {
            info.position[0] = SPA_AUDIO_CHANNEL_MONO;
        } else {
            info.position[0] = SPA_AUDIO_CHANNEL_FL;
            info.position[1] = SPA_AUDIO_CHANNEL_FR;
        }

        const spa_pod* params[1];
        params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

        pw_stream_flags flags = (pw_stream_flags)(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS);

        int res = pw_stream_connect(m_stream, PW_DIRECTION_INPUT, PW_ID_ANY, flags, params, 1);
        if (res < 0) {
            std::cout << "PipeWireLoopbackCapture: pw_stream_connect failed: "
                      << spa_strerror(res) << "\n";
            pw_thread_loop_unlock(m_loop);
            return false;
        }

        pw_thread_loop_unlock(m_loop);
        pw_thread_loop_start(m_loop);

        return true;
    }

    // Call from your main loop, same as WasapiLoopbackCapture::Poll.
    // Appends newly captured samples (already downmixed to mono) onto outSamples.
    void Poll(std::vector<float>& outSamples) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (!m_pendingSamples.empty()) {
            outSamples.insert(outSamples.end(), m_pendingSamples.begin(), m_pendingSamples.end());
            m_pendingSamples.clear();
        }
    }

    void Shutdown() {
        if (m_loop) pw_thread_loop_stop(m_loop);
        if (m_stream) { pw_stream_destroy(m_stream); m_stream = nullptr; }
        if (m_core) { pw_core_disconnect(m_core); m_core = nullptr; }
        if (m_context) { pw_context_destroy(m_context); m_context = nullptr; }
        if (m_loop) { pw_thread_loop_destroy(m_loop); m_loop = nullptr; }
    }

    ~PipeWireLoopbackCapture() { Shutdown(); }

private:
    static void OnStateChanged(void* /*data*/, pw_stream_state /*old*/, pw_stream_state /*state*/, const char* error) {
        if (error) std::cout << "PipeWireLoopbackCapture stream error: " << error << "\n";
    }

    static void OnProcess(void* data) {
        auto* self = (PipeWireLoopbackCapture*)data;
        pw_buffer* b = pw_stream_dequeue_buffer(self->m_stream);
        if (!b) return;

        spa_buffer* buf = b->buffer;
        if (buf->datas[0].data == nullptr) {
            pw_stream_queue_buffer(self->m_stream, b);
            return;
        }

        float* samples = (float*)buf->datas[0].data;
        uint32_t numBytes  = buf->datas[0].chunk->size;
        uint32_t numFloats = numBytes / sizeof(float);
        uint32_t numFrames = numFloats / (uint32_t)self->m_channels;

        {
            std::lock_guard<std::mutex> lock(self->m_bufferMutex);
            self->m_pendingSamples.reserve(self->m_pendingSamples.size() + numFrames);
            for (uint32_t i = 0; i < numFrames; ++i) {
                float mono = 0.0f;
                for (int c = 0; c < self->m_channels; ++c) {
                    mono += samples[i * self->m_channels + c];
                }
                self->m_pendingSamples.push_back(mono / (float)self->m_channels);
            }
        }

        pw_stream_queue_buffer(self->m_stream, b);
    }

    pw_thread_loop* m_loop    = nullptr;
    pw_context*     m_context = nullptr;
    pw_core*        m_core    = nullptr;
    pw_stream*      m_stream  = nullptr;
    spa_hook        m_streamListener{};

    int m_channels = 2;

    std::mutex m_bufferMutex;
    std::vector<float> m_pendingSamples;
};

// Global instance, mirroring the pattern used by g_loopback on Windows.
inline PipeWireLoopbackCapture g_pwLoopback;
