#include <algorithm>
#include <array>
#include <cctype>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>
#include <storm_audio/device.h>
#include <storm_audio/source.h>

#include "al_utils.h"
#include "ogg_decoder.h"
#include "trace_helpers.h"
#include "wav_decoder.h"

using namespace storm::audio;

namespace
{

std::unique_ptr<DataStream> create_compatible_stream(
    std::shared_ptr<DebugTracer> const& tracer, std::filesystem::path const& file_path, DataStream::Format output_format)
{
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (ext == ".ogg") { return std::make_unique<OggDecoder>(tracer, output_format); }
    if (ext == ".wav") { return std::make_unique<WavDecoder>(); }

    return nullptr;
}

}  // namespace

struct Device::Impl {
    Impl(std::shared_ptr<DebugTracer> const& tracer, size_t stream_buffer_count, size_t buffer_sample_count)
        : m_tracer {tracer}
        , m_stream_buffer_count {stream_buffer_count}
        , m_buffer_sample_count {buffer_sample_count}
    {
        m_device = std::shared_ptr<ALCdevice>(alcOpenDevice(nullptr), [](ALCdevice* p) { alcCloseDevice(p); });  // Default device
        if (m_device == nullptr) { return; }

        m_context = std::shared_ptr<ALCcontext>(alcCreateContext(m_device.get(), nullptr), [](ALCcontext* p) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(p);
        });
        ALC_TRACE_ERRORS(tracer, m_device.get());

        if (m_context == nullptr) { return; }

        alcMakeContextCurrent(m_context.get());
        ALC_TRACE_ERRORS(tracer, m_device.get());

        m_out_format = DataStream::Format::Int16;
        m_is_valid   = true;
    }

    ~Impl() = default;

    std::shared_ptr<Sound> create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
    {
        if (!std::filesystem::exists(file_path)) { return nullptr; }

        auto stream = create_compatible_stream(m_tracer, file_path, m_out_format);
        if (!stream || stream->load_file(file_path) != Result::Ok) { return nullptr; }

        return std::make_shared<Sound>(m_tracer, stream, flags);
    }

    std::shared_ptr<SoundStream> create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags)
    {
        if (!std::filesystem::exists(file_path)) { return nullptr; }

        auto stream = create_compatible_stream(m_tracer, file_path, m_out_format);
        if (!stream || stream->load_file(file_path) != Result::Ok) { return nullptr; }

        return std::make_shared<SoundStream>(m_tracer, std::move(stream), flags, m_stream_buffer_count, m_buffer_sample_count);
    }

    std::shared_ptr<Source> attach_sound(std::shared_ptr<Sound> const& sound)
    {
        if (!sound) { return nullptr; }

        auto source = get_free_source();
        source->attach_sound(sound);
        return source;
    }

    std::shared_ptr<Source> attach_sound_stream(std::shared_ptr<SoundStream> const& sound)
    {
        if (!sound) { return nullptr; }

        auto source = get_free_source();
        source->attach_sound_stream(sound);
        return source;
    }

    void set_listener_position_3d(std::array<float, 3> const& position) const
    {
        alListenerfv(AL_POSITION, position.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_listener_velocity_3d(std::array<float, 3> const& velocity) const
    {
        alListenerfv(AL_VELOCITY, velocity.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up) const
    {
        constexpr size_t at_size = sizeof(at);
        constexpr size_t up_size = sizeof(up);

        std::array<float, 6> orientation = {};
        std::memcpy(orientation.data(), at.data(), at_size);
        std::memcpy(orientation.data() + at.size(), up.data(), up_size);

        alListenerfv(AL_ORIENTATION, orientation.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void update()
    {
        for (auto& source: m_sources) {
            source->update();
        }
    }

    std::shared_ptr<Source> get_free_source()
    {
        auto const free_source =
            std::find_if(m_sources.begin(), m_sources.end(), [](auto const& source) { return source->get_state() == SourceState::Free; });

        return free_source != m_sources.end() ? *free_source : create_source();
    }

    std::shared_ptr<Source> create_source()
    {
        m_sources.push_back(std::make_shared<Source>(m_tracer));
        TRACE_INFO(m_tracer, "Add new audio source (current sources count: " + std::to_string(m_sources.size()) + ")");
        return m_sources.back();
    }

    std::shared_ptr<DebugTracer> m_tracer;

    std::shared_ptr<ALCdevice>  m_device;
    std::shared_ptr<ALCcontext> m_context;

    std::vector<std::shared_ptr<Source>> m_sources;

    size_t m_stream_buffer_count;
    size_t m_buffer_sample_count;

    DataStream::Format m_out_format;

    bool m_is_valid;
};

Device::Device(std::shared_ptr<DebugTracer> const& tracer, size_t stream_buffer_count, size_t buffer_sample_count)
    : m_impl {std::make_unique<Impl>(tracer, stream_buffer_count, buffer_sample_count)}
{
}
Device::~Device() = default;

std::shared_ptr<Sound> Device::create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
{
    if (!m_impl->m_is_valid) { return nullptr; }

    return m_impl->create_sound(file_path, flags);
}

std::shared_ptr<SoundStream> Device::create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags)
{
    if (!m_impl->m_is_valid) { return nullptr; }

    return m_impl->create_sound_stream(file_path, flags);
}

std::shared_ptr<Source> Device::attach_sound(std::shared_ptr<Sound> const& sound)
{
    if (!m_impl->m_is_valid) { return {}; }

    return m_impl->attach_sound(sound);
}

std::shared_ptr<Source> Device::attach_sound_stream(std::shared_ptr<SoundStream> const& sound)
{
    if (!m_impl->m_is_valid) { return {}; }

    return m_impl->attach_sound_stream(sound);
}

void Device::set_listener_position_3d(std::array<float, 3> const& position)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_position_3d(position);
}

void Device::set_listener_velocity_3d(std::array<float, 3> const& velocity)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_velocity_3d(velocity);
}

void Device::set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_orientation_3d(at, up);
}

void Device::update()
{
    m_impl->update();
}
