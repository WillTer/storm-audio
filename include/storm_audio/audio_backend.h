#pragma once

#include <array>
#include <filesystem>
#include <memory>

#include "channel.h"
#include "data_stream.h"
#include "sound.h"

namespace storm
{

class IAudioBackend
{
public:
    static std::unique_ptr<IAudioBackend> create_backend();

    virtual ~IAudioBackend() = default;

    virtual bool init() = 0;

    virtual std::shared_ptr<ISound> create_sound(std::filesystem::path const& file_path, ISound::Flags flags) = 0;

    virtual std::weak_ptr<IChannel> bind_sound_to_empty_channel(std::shared_ptr<ISound> const& sound) = 0;
    virtual void                    release_channel(std::weak_ptr<IChannel> const& channel)           = 0;

    virtual void set_listener_position_3d(std::array<float, 3> const& position)       = 0;
    virtual void set_listener_velocity_3d(std::array<float, 3> const& velocity)       = 0;
    virtual void set_listener_orientation_3d(std::array<float, 6> const& orientation) = 0;

    virtual void update() = 0;
};

}  // namespace storm
