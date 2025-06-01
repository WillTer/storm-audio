#pragma once

#include <filesystem>
#include <memory>

#include "sound.h"
#include "sound_stream.h"

namespace storm::audio
{

class Source;
class DebugTracer;

constexpr size_t DEFAULT_BUFFER_COUNT        = 2;
constexpr size_t DEFAULT_BUFFER_SAMPLE_COUNT = 2048;

class Device final
{
public:
    // All models (except None) are clamped
    enum class DistanceModel { None, Inverse, Linear, Exponent };

    Device(
        std::shared_ptr<DebugTracer> const& tracer,
        DistanceModel                       distance_model      = DistanceModel::Inverse,
        size_t                              stream_buffer_count = DEFAULT_BUFFER_COUNT,
        size_t                              buffer_sample_count = DEFAULT_BUFFER_SAMPLE_COUNT);
    ~Device();

    std::shared_ptr<Sound>       create_sound(std::filesystem::path const& file_path, Sound::Flags flags);
    std::shared_ptr<SoundStream> create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags);
    std::shared_ptr<Source>      attach_sound(std::shared_ptr<Sound> const& sound);
    std::shared_ptr<Source>      attach_sound_stream(std::shared_ptr<SoundStream> const& sound);

    void set_listener_position_3d(std::array<float, 3> const& position);
    void set_listener_velocity_3d(std::array<float, 3> const& velocity);
    void set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up);

    void update(std::chrono::milliseconds const& elapsed);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
