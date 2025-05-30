#pragma once

#include <filesystem>
#include <memory>

#include "sound.h"
#include "sound_stream.h"

namespace storm::audio
{

class Source;
class DebugTracer;

class Device final
{
public:
    Device(std::shared_ptr<DebugTracer> const& tracer, size_t stream_buffer_count, size_t buffer_sample_count);
    ~Device();

    std::shared_ptr<Sound>       create_sound(std::filesystem::path const& file_path, Sound::Flags flags);
    std::shared_ptr<SoundStream> create_sound_stream(std::filesystem::path const& file_path, Sound::Flags flags);
    std::shared_ptr<Source>      attach_sound(std::shared_ptr<Sound> const& sound);
    std::shared_ptr<Source>      attach_sound_stream(std::shared_ptr<SoundStream> const& sound);

    void set_listener_position_3d(std::array<float, 3> const& position);
    void set_listener_velocity_3d(std::array<float, 3> const& velocity);
    void set_listener_orientation_3d(std::array<float, 3> const& at, std::array<float, 3> const& up);

    void update();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
