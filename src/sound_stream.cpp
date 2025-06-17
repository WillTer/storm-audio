
#include <atomic>
#include <semaphore>

#include <storm_audio/sound_stream.h>

#include "al_utils.h"
#include "format_helpers.h"

using namespace storm::audio;

struct SoundStream::Impl {
    Impl(
        TraceFunction const&          trace_func,
        std::unique_ptr<DataStream>&& stream,
        Sound::Flags                  flags,
        size_t                        stream_buffer_count,
        size_t                        buffer_sample_count)
        : m_trace_func {trace_func}
        , m_stream {std::move(stream)}
        , m_position {std::chrono::milliseconds(0)}
        , m_flags {flags}
        , m_buffer_sample_count {buffer_sample_count}
        , m_attached_source {0}
        , m_buffer_lock {1}
    {
        m_buffers.resize(stream_buffer_count);

        alGenBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS(m_trace_func);

        m_sample_rate = m_stream->get_sample_rate();
        m_channels    = m_stream->get_channels();
        m_al_format   = convert_to_al_format(m_stream->get_data_format(), m_channels);

        m_stream->seek_start();
        prepare_buffer_data();
    }

    ~Impl()
    {
        detach_source();

        alDeleteBuffers(static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void attach_source(unsigned source)
    {
        if (source == 0) { return; }

        detach_source();  // detach previous source

        m_attached_source.store(source);

        alSourceQueueBuffers(source, static_cast<int>(m_buffers.size()), m_buffers.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void detach_source()
    {
        auto const source = m_attached_source.load();
        if (source == 0) { return; }

        alSourceStop(source);
        AL_TRACE_ERRORS(m_trace_func);

        alSourcei(source, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_trace_func);

        m_attached_source.store(0);
    }

    void prepare_buffer_data()
    {
        for (auto const buffer: m_buffers) {
            update_buffer(buffer, false);
        }
    }

    bool update_buffer(unsigned buffer, bool is_looping)
    {
        m_buffer_lock.acquire();

        auto samples = m_stream->get_samples(m_buffer_data, m_buffer_sample_count);

        // If nothing read, and we're looping, read again, but from the start
        if (samples == 0 && is_looping) {
            m_stream->seek_start();
            samples = m_stream->get_samples(m_buffer_data, m_buffer_sample_count);
        }

        // If still nothing read (or we're not looping), then just stop
        if (samples == 0) {
            m_buffer_lock.release();
            return false;
        }

        alBufferData(buffer, m_al_format, m_buffer_data.data(), static_cast<int>(m_buffer_data.size()), m_sample_rate);
        AL_TRACE_ERRORS(m_trace_func);

        m_buffer_lock.release();

        return true;
    }

    size_t update(bool is_looping)
    {
        auto const source = m_attached_source.load();
        if (source == 0) { return 0; }

        // TODO: Calculate which buffer are playing now to get precise position
        m_position = m_stream->get_buffer_position();

        ALint processed = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
        AL_TRACE_ERRORS(m_trace_func);

        size_t updated = 0;
        for (ALint i = 0; i < processed; ++i) {
            ALuint buffer = 0;
            alSourceUnqueueBuffers(source, 1, &buffer);
            AL_TRACE_ERRORS(m_trace_func);

            if (update_buffer(buffer, is_looping)) {
                alSourceQueueBuffers(source, 1, &buffer);
                AL_TRACE_ERRORS(m_trace_func);
                ++updated;
            }
        }

        return updated;
    }

    void set_stream_buffer_position(std::chrono::milliseconds const& pos)
    {
        auto const source = m_attached_source.load();

        // We need to detach buffers from source to update the ones that are in-use
        detach_source();
        m_position = pos;
        m_stream->set_buffer_position(m_position);
        prepare_buffer_data();
        attach_source(source);
    }

    std::chrono::milliseconds get_stream_buffer_position() const
    {
        return m_position;
    }

    TraceFunction               m_trace_func;
    std::unique_ptr<DataStream> m_stream;

    std::chrono::milliseconds m_position;

    Sound::Flags m_flags;

    size_t m_buffer_sample_count;

    ALenum m_al_format;
    int    m_channels;
    int    m_sample_rate;

    std::vector<char> m_buffer_data;

    std::vector<unsigned> m_buffers;
    std::atomic<unsigned> m_attached_source;

    std::binary_semaphore m_buffer_lock;
};

SoundStream::SoundStream(
    TraceFunction const&          trace_func,
    std::unique_ptr<DataStream>&& stream,
    Sound::Flags                  flags,
    size_t                        stream_buffer_count,
    size_t                        buffer_sample_count)
    : m_impl {std::make_unique<Impl>(trace_func, std::move(stream), flags, stream_buffer_count, buffer_sample_count)}
{
}

SoundStream::~SoundStream() = default;

void SoundStream::attach_source(unsigned source)
{
    m_impl->attach_source(source);
}

void SoundStream::detach_source()
{
    m_impl->detach_source();
}

int SoundStream::get_channels() const
{
    return m_impl->m_channels;
}

size_t SoundStream::update(bool is_looping)
{
    return m_impl->update(is_looping);
}

void SoundStream::set_stream_buffer_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_stream_buffer_position(pos);
}

std::chrono::milliseconds SoundStream::get_stream_buffer_position() const
{
    return m_impl->get_stream_buffer_position();
}

Sound::Flags SoundStream::get_flags() const
{
    return m_impl->m_flags;
}
