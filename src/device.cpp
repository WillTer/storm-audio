#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <format>
#include <ranges>
#include <semaphore>
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

std::unique_ptr<DataStream>
create_compatible_stream(TraceFunction const& trace_func, std::filesystem::path const& file_path, DataStream::Format output_format)
{
    std::string ext = file_path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char const ch) { return static_cast<char>(std::tolower(ch)); });

    if (ext == ".ogg") { return std::make_unique<OggDecoder>(trace_func, output_format); }
    if (ext == ".wav") { return std::make_unique<WavDecoder>(); }

    return nullptr;
}

void no_trace(
    MessageSeverity /*severity*/,
    std::string const& /*message*/,
    std::filesystem::path const& /*source_file*/,
    size_t /*line*/,
    std::string const& /*function_name*/)
{
}

}  // namespace

struct Device::Impl {
    Impl(TraceFunction const& func, DistanceModel distance_model, size_t stream_buffer_count, size_t buffer_sample_count)
        : m_trace_func {func ? func : no_trace}
        , m_stream_buffer_count {stream_buffer_count}
        , m_buffer_sample_count {buffer_sample_count}
        , m_sources_lock {1}
    {
        m_is_valid.store(false);

        m_device = std::shared_ptr<ALCdevice>(alcOpenDevice(nullptr), [](ALCdevice* p) { alcCloseDevice(p); });  // Default device
        if (m_device == nullptr) { return; }

        m_context = std::shared_ptr<ALCcontext>(alcCreateContext(m_device.get(), nullptr), [](ALCcontext* p) {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(p);
        });
        ALC_TRACE_ERRORS(m_trace_func, m_device.get());

        if (m_context == nullptr) { return; }

        alcMakeContextCurrent(m_context.get());
        ALC_TRACE_ERRORS(m_trace_func, m_device.get());

        switch (distance_model) {
        case DistanceModel::None: alDistanceModel(AL_NONE); break;
        case DistanceModel::Inverse: alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED); break;
        case DistanceModel::Linear: alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED); break;
        case DistanceModel::Exponent: alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED); break;
        }
        AL_TRACE_ERRORS(m_trace_func);

        m_out_format = DataStream::Format::Int16;

        m_is_valid.store(true);
    }

    ~Impl() = default;

    std::shared_ptr<Sound> create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
    {
        if (!std::filesystem::exists(file_path)) { return nullptr; }

        auto stream = create_compatible_stream(m_trace_func, file_path, m_out_format);
        if (!stream || stream->load_file(file_path) != Result::Ok) { return nullptr; }

        return std::make_shared<Sound>(m_trace_func, stream, flags);
    }

    std::shared_ptr<SoundStream> create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags)
    {
        if (!std::filesystem::exists(file_path)) { return nullptr; }

        auto stream = create_compatible_stream(m_trace_func, file_path, m_out_format);
        if (!stream || stream->load_file(file_path) != Result::Ok) { return nullptr; }

        return std::make_shared<SoundStream>(m_trace_func, std::move(stream), flags, m_stream_buffer_count, m_buffer_sample_count);
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
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_listener_velocity_3d(std::array<float, 3> const& velocity) const
    {
        alListenerfv(AL_VELOCITY, velocity.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up) const
    {
        constexpr size_t at_size = sizeof(at);
        constexpr size_t up_size = sizeof(up);

        std::array<float, 6> orientation = {};
        std::memcpy(orientation.data(), at.data(), at_size);
        std::memcpy(orientation.data() + at.size(), up.data(), up_size);

        alListenerfv(AL_ORIENTATION, orientation.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void update(std::chrono::milliseconds const& elapsed)
    {
        if (!m_sources_lock.try_acquire()) { return; }  // Do not lock on update
        for (auto const& source: m_sources) {
            source->update(elapsed);
        }
        m_sources_lock.release();
    }

    std::shared_ptr<Source> get_free_source()
    {
        m_sources_lock.acquire();

        auto const create_source = [this]() {
            m_sources.push_back(std::make_shared<Source>(m_trace_func));
            TRACE_INFO(m_trace_func, std::format("Add new audio source (current sources count: {})", m_sources.size()));
            return m_sources.back();
        };

        auto const free_source =
            std::ranges::find_if(m_sources, [](auto const& source) { return source->get_state() == SourceState::Free; });
        auto const ptr = free_source != m_sources.end() ? *free_source : create_source();

        m_sources_lock.release();

        return ptr;
    }

    TraceFunction m_trace_func;

    std::shared_ptr<ALCdevice>  m_device;
    std::shared_ptr<ALCcontext> m_context;

    std::vector<std::shared_ptr<Source>> m_sources;

    size_t m_stream_buffer_count;
    size_t m_buffer_sample_count;

    DataStream::Format m_out_format;

    std::atomic_bool m_is_valid;

    std::binary_semaphore m_sources_lock;
};

Device::Device(
    TraceFunction const& func /*{}*/,
    DistanceModel        distance_model /*= DistanceModel::Inverse*/,
    size_t               stream_buffer_count /*= DEFAULT_BUFFER_COUNT*/,
    size_t               buffer_sample_count /*= DEFAULT_BUFFER_SAMPLE_COUNT*/)
    : m_impl {std::make_unique<Impl>(func, distance_model, stream_buffer_count, buffer_sample_count)}
{
}

Device::~Device() = default;

std::shared_ptr<Sound> Device::create_sound(std::filesystem::path const& file_path, Sound::Flags flags)
{
    if (!m_impl->m_is_valid.load()) { return nullptr; }

    return m_impl->create_sound(file_path, flags);
}

std::shared_ptr<SoundStream> Device::create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags)
{
    if (!m_impl->m_is_valid.load()) { return nullptr; }

    return m_impl->create_sound_stream(file_path, flags);
}

std::shared_ptr<Source> Device::attach_sound(std::shared_ptr<Sound> const& sound)
{
    if (!m_impl->m_is_valid.load()) { return {}; }

    return m_impl->attach_sound(sound);
}

std::shared_ptr<Source> Device::attach_sound_stream(std::shared_ptr<SoundStream> const& sound)
{
    if (!m_impl->m_is_valid.load()) { return {}; }

    return m_impl->attach_sound_stream(sound);
}

void Device::set_listener_position_3d(std::array<float, 3> const& position)
{
    if (!m_impl->m_is_valid.load()) { return; }

    m_impl->set_listener_position_3d(position);
}

void Device::set_listener_velocity_3d(std::array<float, 3> const& velocity)
{
    if (!m_impl->m_is_valid.load()) { return; }

    m_impl->set_listener_velocity_3d(velocity);
}

void Device::set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up)
{
    if (!m_impl->m_is_valid.load()) { return; }

    m_impl->set_listener_orientation_3d(at, up);
}

void Device::update(std::chrono::milliseconds const& elapsed)
{
    m_impl->update(elapsed);
}
