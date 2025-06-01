#pragma once

#include <array>
#include <chrono>
#include <memory>

namespace storm::audio
{

enum class SourceState { Free, Playing, Paused, Stopped };

class Sound;
class SoundStream;
class DebugTracer;

class Source final
{
public:
    Source(std::shared_ptr<DebugTracer> const& tracer);
    ~Source();

    void play();
    void pause();
    void stop();

    SourceState get_state() const;

    void                      set_playback_position(std::chrono::milliseconds const& pos);
    std::chrono::milliseconds get_playback_position() const;

    void set_min_distance(float distance);
    void set_max_distance(float distance);

    void set_position_3d(std::array<float, 3> const& position);
    void set_velocity_3d(std::array<float, 3> const& velocity);
    void set_direction_3d(std::array<float, 3> const& direction);

    void  set_volume(float volume_level);
    float get_volume() const;

    void  set_volume_min(float volume_min);
    float get_volume_min() const;

    void  set_volume_max(float volume_max);
    float get_volume_max() const;

    void  set_pitch(float pitch_level);
    float get_pitch() const;

    void set_looping(bool flag);

    void fade(float start_volume, float end_volume, std::chrono::milliseconds const& duration);

private:
    friend class Device;

    void attach_sound(std::shared_ptr<Sound> const& sound);
    void attach_sound_stream(std::shared_ptr<SoundStream> const& sound);
    void detach_sound();

    void update(std::chrono::milliseconds const& elapsed);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
