#pragma once

#include <memory>

#include <storm_audio/audio_backend.h>

namespace storm
{

class ALBackend: virtual public IAudioBackend
{
public:
    ALBackend();
    ~ALBackend() override;

    bool init() override;

    std::shared_ptr<ISound> create_sound(std::filesystem::path const& file_path, ISound::Flags flags) override;

    std::weak_ptr<IChannel> bind_sound_to_empty_channel(std::shared_ptr<ISound> const& sound) override;
    void                    release_channel(std::weak_ptr<IChannel> const& channel) override;

    void set_listener_position_3d(std::array<float, 3> const& position) override;
    void set_listener_velocity_3d(std::array<float, 3> const& velocity) override;
    void set_listener_orientation_3d(std::array<float, 6> const& orientation) override;

    void update() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm
