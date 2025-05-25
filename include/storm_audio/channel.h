#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>

#include "debug_tracer.h"

namespace storm::audio
{

enum class ChannelState { None, Initial, Playing, Paused, Stopped };

class Sound;

class Channel final
{
public:
    Channel(std::shared_ptr<DebugTracer> const& tracer);
    ~Channel();

    void play();
    void pause();
    void stop();

    ChannelState get_state() const;

    void                      set_playback_position(std::chrono::milliseconds const& pos);
    std::chrono::milliseconds get_playback_position() const;

    void set_min_distance(float distance);
    void set_max_distance(float distance);

    void set_position_3d(std::array<float, 3> const& position);
    void set_velocity_3d(std::array<float, 3> const& velocity);
    void set_direction_3d(std::array<float, 3> const& orientation);

    void  set_volume(float volume_level);
    float get_volume() const;

    void  set_pitch(float pitch_level);
    float get_pitch() const;

    void set_looping(bool flag);

private:
    friend class Backend;

    void attach_sound(std::shared_ptr<Sound> const& sound);
    void detach_sound();

    void internal_update();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
