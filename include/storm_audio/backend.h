#pragma once

#include <filesystem>
#include <memory>

#include "channel.h"
#include "data_stream.h"
#include "debug_tracer.h"
#include "sound.h"

namespace storm::audio
{

class Backend final
{
public:
    Backend(std::shared_ptr<DebugTracer> const& tracer);
    ~Backend();

    std::shared_ptr<Sound> create_sound(std::filesystem::path const& file_path, Sound::Flags flags);

    Channel* attach_sound(std::shared_ptr<Sound> const& sound);

    void set_listener_position_3d(std::array<float, 3> const& position);
    void set_listener_velocity_3d(std::array<float, 3> const& velocity);
    void set_listener_orientation_3d(std::array<float, 6> const& orientation);

    void update();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
