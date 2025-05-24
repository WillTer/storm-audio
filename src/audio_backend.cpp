#include <algorithm>
#include <array>
#include <cctype>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>
#include <storm_audio/backend.h>

#include "al_utils.h"
#include "const.h"
#include "trace_helpers.h"
#include "wav_decoder.h"

using namespace storm::audio;

namespace
{

std::unique_ptr<AbstractDataStream>
create_compatible_stream(std::filesystem::path const& file_path, [[maybe_unused]] AbstractDataStream::Format output_format)
{
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    // if (ext == ".ogg") { return std::make_unique<VorbisDecoder>(output_format); }
    if (ext == ".wav") { return std::make_unique<WavDecoder>(); }

    return nullptr;
}

}  // namespace

struct Backend::Impl {
    Impl(std::shared_ptr<DebugTracer> const& tracer) : m_tracer {tracer}
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

        m_out_format =
            (alIsExtensionPresent(FLOAT_EXT_NAME) == AL_TRUE) ? AbstractDataStream::Format::Float32 : AbstractDataStream::Format::Int16;
        m_is_valid = true;
    }

    ~Impl() = default;

    std::shared_ptr<Sound> create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
    {
        if (!std::filesystem::exists(file_path)) { return nullptr; }

        auto stream = create_compatible_stream(file_path, m_out_format);
        if (!stream || stream->load_file(file_path) != Result::Ok) { return nullptr; }

        return std::make_shared<Sound>(m_tracer, std::move(stream), flags);
    }

    Channel* attach_sound(std::shared_ptr<Sound> const& sound)
    {
        if (!sound) { return nullptr; }

        Channel* channel = nullptr;

        auto const free_channel =
            std::find_if(m_channels.begin(), m_channels.end(), [](auto const& ch) { return ch->get_state() == ChannelState::Stopped; });

        if (free_channel != m_channels.end()) {
            channel = free_channel->get();
            channel->detach_sound();
        } else {
            TRACE_INFO(m_tracer, "Add new audio channel (current channels count: " + std::to_string(m_channels.size()) + ")");
            m_channels.push_back(std::make_unique<Channel>(m_tracer));
            channel = m_channels.back().get();
        }

        channel->attach_sound(sound);
        return channel;
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

    void set_listener_orientation_3d(std::array<float, 6> const& orientation) const
    {
        alListenerfv(AL_ORIENTATION, orientation.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void update()
    {
        for (auto& channel: m_channels) {
            channel->internal_update();
        }
    }

    std::shared_ptr<DebugTracer> m_tracer;

    std::shared_ptr<ALCdevice>  m_device;
    std::shared_ptr<ALCcontext> m_context;

    std::vector<std::unique_ptr<Channel>> m_channels;

    AbstractDataStream::Format m_out_format;

    bool m_is_valid;
};

Backend::Backend(std::shared_ptr<DebugTracer> const& tracer) : m_impl {std::make_unique<Impl>(tracer)} {}
Backend::~Backend() = default;

std::shared_ptr<Sound> Backend::create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
{
    if (!m_impl->m_is_valid) { return nullptr; }

    return m_impl->create_sound(file_path, flags);
}

Channel* Backend::attach_sound(std::shared_ptr<Sound> const& sound)
{
    if (!m_impl->m_is_valid) { return {}; }

    return m_impl->attach_sound(sound);
}

void Backend::set_listener_position_3d(std::array<float, 3> const& position)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_position_3d(position);
}

void Backend::set_listener_velocity_3d(std::array<float, 3> const& velocity)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_velocity_3d(velocity);
}

void Backend::set_listener_orientation_3d(std::array<float, 6> const& orientation)
{
    if (!m_impl->m_is_valid) { return; }

    m_impl->set_listener_orientation_3d(orientation);
}

void Backend::update()
{
    m_impl->update();
}
