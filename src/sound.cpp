#include <algorithm>
#include <cstring>
#include <semaphore>

#include <storm_audio/sound.h>

#include "al_utils.h"
#include "format_helpers.h"

using namespace storm::audio;

struct Sound::Impl {
    Impl(TraceFunction const& trace_func, std::unique_ptr<DataStream> const& stream, Flags flags)
        : m_trace_func {trace_func}
        , m_flags {flags}
        , m_sources_lock {1}
    {
        alGenBuffers(1, &m_buffer);
        AL_TRACE_ERRORS(m_trace_func);

        m_sample_rate = stream->get_sample_rate();
        m_channels    = is_flag_enabled(m_flags, Flags::Spatial3D) ? 1 : stream->get_channels();
        m_al_format   = convert_to_al_format(stream->get_data_format(), m_channels);
        m_sample_size = get_format_sample_size(stream->get_data_format());

        prepare_buffer_data(stream, stream->get_channels());
    }

    ~Impl()
    {
        detach_all_sources();

        alDeleteBuffers(1, &m_buffer);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void attach_source(unsigned source)
    {
        alSourcei(source, AL_BUFFER, m_buffer);
        AL_TRACE_ERRORS(m_trace_func);

        m_sources_lock.acquire();
        m_attached_sources.push_back(source);
        m_sources_lock.release();
    }

    void detach_source(unsigned source)
    {
        m_sources_lock.acquire();
        auto it = std::find_if(m_attached_sources.begin(), m_attached_sources.end(), [&source](unsigned const s) { return s == source; });

        if (it == m_attached_sources.end()) {
            m_sources_lock.release();
            return;
        }

        alSourcei(*it, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_trace_func);

        m_attached_sources.erase(it);
        m_sources_lock.release();
    }

    void prepare_buffer_data(std::unique_ptr<DataStream> const& stream, int pcm_channels)
    {
        std::vector<char> buffer = {};

        auto const samples = stream->get_samples_all(buffer);
        if (samples == 0) { return; }

        // OpenAL doesn't support 3D effects for audio with more than one channel, so remove excess channel data manually
        if (is_flag_enabled(m_flags, Flags::Spatial3D) && pcm_channels > 1) {
            std::vector<char> mono_buffer = {};
            mono_buffer.resize(buffer.size() / pcm_channels);

            // No mixing, just use left channel data for simplicity
            for (size_t offset = 0; offset < mono_buffer.size(); offset += m_sample_size) {
                std::memcpy(mono_buffer.data() + offset, buffer.data() + (offset * m_sample_size), m_sample_size);
            }
            mono_buffer.swap(buffer);
        }

        alBufferData(m_buffer, m_al_format, buffer.data(), static_cast<int>(buffer.size()), m_sample_rate);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void detach_all_sources()
    {
        m_sources_lock.acquire();
        for (auto const source: m_attached_sources) {
            alSourceStop(source);
            AL_TRACE_ERRORS(m_trace_func);

            alSourcei(source, AL_BUFFER, 0);
            AL_TRACE_ERRORS(m_trace_func);
        }

        // Clear sources list, as we're not binded to them anymore
        m_attached_sources.clear();
        m_sources_lock.release();
    }

    TraceFunction m_trace_func;

    Flags m_flags;

    ALenum m_al_format;
    int    m_channels;
    int    m_sample_rate;

    size_t m_sample_size;

    unsigned              m_buffer;
    std::vector<unsigned> m_attached_sources;

    std::binary_semaphore m_sources_lock;
};

Sound::Sound(TraceFunction const& trace_func, std::unique_ptr<DataStream> const& stream, Flags flags)
    : m_impl {std::make_unique<Impl>(trace_func, stream, flags)}
{
}

Sound::~Sound() = default;

void Sound::attach_source(unsigned source)
{
    m_impl->attach_source(source);
}

void Sound::detach_source(unsigned source)
{
    m_impl->detach_source(source);
}

int Sound::get_channels() const
{
    return m_impl->m_channels;
}

Sound::Flags Sound::get_flags() const
{
    return m_impl->m_flags;
}
