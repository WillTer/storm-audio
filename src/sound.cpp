
#include <algorithm>

#include <storm_audio/sound.h>

#include "al_utils.h"
#include "format_helpers.h"

using namespace storm::audio;

struct Sound::Impl {
    Impl(std::shared_ptr<DebugTracer> const& tracer, std::unique_ptr<DataStream> const& stream, Flags flags)
        : m_tracer {tracer}
        , m_flags {flags}
    {
        alGenBuffers(1, &m_buffer);
        AL_TRACE_ERRORS(m_tracer);

        m_sample_rate = stream->get_sample_rate();
        m_channels    = stream->get_channels();
        m_al_format   = convert_to_al_format(stream->get_data_format(), m_channels);

        prepare_buffer_data(stream);
    }

    ~Impl()
    {
        detach_all_sources();

        alDeleteBuffers(1, &m_buffer);
        AL_TRACE_ERRORS(m_tracer);
    }

    void attach_source(unsigned source)
    {
        alSourcei(source, AL_BUFFER, m_buffer);
        AL_TRACE_ERRORS(m_tracer);

        m_attached_sources.push_back(source);
    }

    void detach_source(unsigned source)
    {
        auto it = std::find_if(m_attached_sources.begin(), m_attached_sources.end(), [&source](unsigned const s) { return s == source; });

        if (it == m_attached_sources.end()) { return; }

        alSourcei(*it, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_tracer);

        m_attached_sources.erase(it);
    }

    void prepare_buffer_data(std::unique_ptr<DataStream> const& stream)
    {
        std::vector<char> buffer = {};

        auto const samples = stream->get_samples_all(buffer);
        if (samples == 0) { return; }

        alBufferData(m_buffer, m_al_format, buffer.data(), static_cast<int>(buffer.size()), m_sample_rate);
        AL_TRACE_ERRORS(m_tracer);
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

    Flags m_flags;

    ALenum m_al_format;
    int    m_channels;
    int    m_sample_rate;

    unsigned              m_buffer;
    std::vector<unsigned> m_attached_sources;
};

Sound::Sound(std::shared_ptr<DebugTracer> const& tracer, std::unique_ptr<DataStream> const& stream, Flags flags)
    : m_impl {std::make_unique<Impl>(tracer, stream, flags)}
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
