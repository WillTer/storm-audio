
#include <algorithm>

#include <storm_audio/sound.h>

#include "al_utils.h"
#include "format_helpers.h"

using namespace storm::audio;

struct Sound::Impl {
    Impl(
        std::shared_ptr<DebugTracer> const& tracer,
        std::unique_ptr<DataStream>&&       stream,
        Flags                               flags,
        size_t                              stream_buffer_count,
        size_t                              buffer_sample_count)
        : m_tracer {tracer}
        , m_stream {std::move(stream)}
        , m_flags {flags}
        , m_stream_buffer_count {stream_buffer_count}
        , m_buffer_sample_count {buffer_sample_count}
    {
        m_buffers.resize(is_flag_enabled(flags, Flags::Stream) ? m_stream_buffer_count : 1);

        alGenBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS(m_tracer);

        m_sample_rate = m_stream->get_sample_rate();
        m_channels    = m_stream->get_channels();
        m_data_format = m_stream->get_data_format();
        m_al_format   = convert_to_al_format(m_data_format, m_channels);
        m_sample_size = get_format_sample_size(m_data_format);

        m_stream->seek_start();
        prepare_buffer_data();
    }

    ~Impl()
    {
        detach_all_sources();

        alDeleteBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void attach_buffers(unsigned source)
    {
        if (is_flag_enabled(m_flags, Flags::Stream)) {
            alSourceQueueBuffers(source, static_cast<int>(m_buffers.size()), m_buffers.data());
        } else {
            alSourcei(source, AL_BUFFER, m_buffers[0]);
        }
        AL_TRACE_ERRORS(m_tracer);

        m_attached_sources.push_back(source);
    }

    void detach_buffers(unsigned source)
    {
        auto it = std::find_if(m_attached_sources.begin(), m_attached_sources.end(), [&source](unsigned const s) { return s == source; });

        if (it == m_attached_sources.end()) { return; }

        alSourcei(*it, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_tracer);

        m_attached_sources.erase(it);
    }

    void prepare_buffer_data_full()
    {
        std::vector<char> buffer = {};

        auto const samples = m_stream->get_samples_all(buffer);
        if (samples == 0) { return; }

        alBufferData(m_buffers[0], m_al_format, buffer.data(), static_cast<int>(buffer.size()), m_sample_rate);
        AL_TRACE_ERRORS(m_tracer);
    }

    void prepare_buffer_data_stream()
    {
        for (auto const buffer: m_buffers) {
            update_buffer(buffer, false);
        }
    }

    bool update_buffer(unsigned buffer, bool is_looping)
    {
        auto samples = m_stream->get_samples(m_buffer_data, m_buffer_sample_count);

        // If nothing read and we're looping, read again, but from the start
        if (samples == 0 && is_looping) {
            m_stream->seek_start();
            samples = m_stream->get_samples(m_buffer_data, m_buffer_sample_count);
        }

        // If still nothing read (or we're not looping), then just stop
        if (samples == 0) { return false; }

        alBufferData(buffer, m_al_format, m_buffer_data.data(), static_cast<int>(m_buffer_data.size()), m_sample_rate);
        AL_TRACE_ERRORS(m_tracer);

        return true;
    }

    void prepare_buffer_data()
    {
        if (is_flag_enabled(m_flags, Flags::Stream)) {
            prepare_buffer_data_stream();
        } else {
            prepare_buffer_data_full();
        }
    }

    void set_stream_buffer_position(std::chrono::milliseconds const& pos)
    {
        m_stream->set_buffer_position(pos);
        prepare_buffer_data_stream();
    }

    std::chrono::milliseconds get_stream_buffer_position() const
    {
        return m_stream->get_buffer_position();
    }

    void detach_all_sources()
    {
        for (auto const source: m_attached_sources) {
            alSourceStop(source);
            AL_TRACE_ERRORS(m_tracer);

            alSourcei(source, AL_BUFFER, 0);
            AL_TRACE_ERRORS(m_tracer);
        }

        // Clear sources list, as we're not binded to them anymore
        m_attached_sources.clear();
    }

    std::shared_ptr<DebugTracer> m_tracer;
    std::unique_ptr<DataStream>  m_stream;

    Flags              m_flags;
    DataStream::Format m_data_format;

    size_t m_stream_buffer_count;
    size_t m_buffer_sample_count;

    ALenum m_al_format;
    int    m_channels;
    int    m_sample_rate;

    size_t m_sample_size;

    std::vector<char> m_buffer_data;

    std::vector<unsigned> m_buffers;
    std::vector<unsigned> m_attached_sources;
};

Sound::Sound(
    std::shared_ptr<DebugTracer> const& tracer,
    std::unique_ptr<DataStream>&&       stream,
    Flags                               flags,
    size_t                              stream_buffer_count,
    size_t                              buffer_sample_count)
    : m_impl {std::make_unique<Impl>(tracer, std::move(stream), flags, stream_buffer_count, buffer_sample_count)}
{
}

Sound::~Sound() = default;

void Sound::attach_buffers(unsigned source)
{
    m_impl->attach_buffers(source);
}

void Sound::detach_buffers(unsigned source)
{
    m_impl->detach_buffers(source);
}

int Sound::get_channels() const
{
    return m_impl->m_channels;
}

bool Sound::update_buffer(unsigned buffer, bool is_looping)
{
    return m_impl->update_buffer(buffer, is_looping);
}

void Sound::set_stream_buffer_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_stream_buffer_position(pos);
}

std::chrono::milliseconds Sound::get_stream_buffer_position() const
{
    return m_impl->get_stream_buffer_position();
}

Sound::Flags Sound::get_flags() const
{
    return m_impl->m_flags;
}
