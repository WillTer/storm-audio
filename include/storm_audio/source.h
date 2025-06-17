#pragma once

#include <array>
#include <chrono>
#include <memory>

namespace storm::audio
{

enum class SourceState { Free, Playing, Paused, Stopped };

class Sound;
class SoundStream;
class Debugtrace_func;

class Source final
{
public:
    Source(TraceFunction const& trace_func);
    ~Source();

    void play();
    void pause();
    void stop();

    void play_with_fade(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {});
    void pause_with_fade(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {});
    void stop_with_fade(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {});

    SourceState get_state() const;

    void                      set_playback_position(std::chrono::milliseconds const& pos);
    std::chrono::milliseconds get_playback_position() const;

    void set_min_distance(float distance);
    void set_max_distance(float distance);

    void set_position_3d(std::array<float, 3> const& position);
    void set_velocity_3d(std::array<float, 3> const& velocity);
    void set_direction_3d(std::array<float, 3> const& direction);

    void  set_rolloff(float factor);
    float get_rolloff() const;

    void  set_volume(float volume_level);
    float get_volume() const;

    void  set_volume_min(float volume_min);
    float get_volume_min() const;

    void  set_volume_max(float volume_max);
    float get_volume_max() const;

    void  set_pitch(float pitch_level);
    float get_pitch() const;

    void set_looping(bool flag);

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
