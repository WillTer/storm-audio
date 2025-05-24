
#include <storm_audio/sound.h>

#include <algorithm>

#include "al_utils.h"
#include "const.h"
#include "format_helpers.h"

using namespace storm;

struct Sound::Impl {
    Impl(std::unique_ptr<AbstractDataStream>&& stream, Flags flags) : m_stream {std::move(stream)}, m_flags {flags}
    {
        m_buffers.resize(has_flag(flags, Stream) ? STREAM_BUFFER_COUNT : 1);

        alGenBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS();

        m_sample_rate = m_stream->get_sample_rate();
        m_channels    = m_stream->get_channels();
        m_data_format = m_stream->get_data_format();
        m_al_format   = convert_to_al_format(m_data_format, m_channels);
        m_sample_size = get_format_sample_size(m_data_format);

        reset_buffers();
    }

    ~Impl()
    {
        unbind_sources();

        alDeleteBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS();
    }

    void bind_buffers_to_source(unsigned source)
    {
        if (has_flag(m_flags, Stream)) {
            alSourceQueueBuffers(source, static_cast<int>(m_buffers.size()), m_buffers.data());
        } else {
            alSourcei(source, AL_BUFFER, m_buffers[0]);
        }
        AL_TRACE_ERRORS();

        m_binded_sources.push_back(source);
    }

    void unbind_source(unsigned source)
    {
        auto it = std::find_if(m_binded_sources.begin(), m_binded_sources.end(), [&source](unsigned const s) { return s == source; });

        if (it == m_binded_sources.end()) { return; }

        alSourcei(*it, AL_BUFFER, 0);
        AL_TRACE_ERRORS();

        m_binded_sources.erase(it);
    }

    void bind_buffer_data()
    {
        std::vector<uint8_t> buffer = {};

        auto const samples = m_stream->get_samples_all(buffer);
        if (samples == 0) { return; }

        alBufferData(m_buffers[0], m_al_format, buffer.data(), static_cast<int>(buffer.size()), m_sample_rate);
        AL_TRACE_ERRORS();
    }

    void bind_buffer_data_stream()
    {
        for (auto const buffer: m_buffers) {
            fetch_samples_for_buffer(buffer, false);
        }
    }

    bool fetch_samples_for_buffer(unsigned buffer, bool is_looping)
    {
        m_buffer_data.resize(BUFFER_SAMPLE_COUNT * m_sample_size);
        auto samples = m_stream->get_samples(m_buffer_data, BUFFER_SAMPLE_COUNT);

        if (samples == 0 && is_looping) {
            m_stream->seek_start();
            samples = m_stream->get_samples(m_buffer_data, BUFFER_SAMPLE_COUNT);
        }

        if (samples == 0) { return false; }

        alBufferData(buffer, m_al_format, m_buffer_data.data(), static_cast<int>(m_buffer_data.size()), m_sample_rate);
        AL_TRACE_ERRORS();

        return true;
    }

    void reset_buffers()
    {
        // Unbind buffers from all binded sources
        unbind_sources();

        m_stream->seek_start();

        if (has_flag(m_flags, Stream)) {
            bind_buffer_data_stream();
        } else {
            bind_buffer_data();
        }
    }

    void unbind_sources()
    {
        for (auto const source: m_binded_sources) {
            alSourceStop(source);
            AL_TRACE_ERRORS();

            alSourcei(source, AL_BUFFER, 0);
            AL_TRACE_ERRORS();
        }

        // Clear sources list, as we're not binded to them anymore
        m_binded_sources.clear();
    }

    std::unique_ptr<AbstractDataStream> m_stream;

    Flags               m_flags;
    AbstractDataStream::Format m_data_format;

    ALenum m_al_format;
    int    m_channels;
    int    m_sample_rate;

    size_t m_sample_size;

    std::vector<uint8_t> m_buffer_data;

    std::vector<unsigned> m_buffers;
    std::vector<unsigned> m_binded_sources;
};

Sound::Sound(std::unique_ptr<AbstractDataStream>&& stream, Flags flags) : m_impl {std::make_unique<Impl>(std::move(stream), flags)} {}
Sound::~Sound() = default;

void Sound::bind_buffers_to_source(unsigned source)
{
    m_impl->bind_buffers_to_source(source);
}

void Sound::unbind_source(unsigned source)
{
    m_impl->unbind_source(source);
}

int Sound::get_channels() const
{
    return m_impl->m_channels;
}

bool Sound::push_next_data(unsigned buffer, bool is_looping) const
{
    return m_impl->fetch_samples_for_buffer(buffer, is_looping);
}

void Sound::reset_buffers()
{
    m_impl->reset_buffers();
}

Sound::Flags Sound::get_flags() const
{
    return m_impl->m_flags;
}
