#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>

namespace storm
{

// TODO: Rewrite error handling
enum class Result : int32_t {
    Ok                        = 0,
    ErrNotInitialized         = -1,
    ErrInvalidArgument        = -2,
    ErrFileNotFound           = -3,
    ErrFileOpenFailed         = -4,
    ErrFileFormatInvalid      = -5,
    ErrFileFormatNotSupported = -6,
    ErrChannelIsEmpty         = -7,
    ErrNoEmptyChannels        = -8,
    ErrInternal               = -9,

    ErrUnknown = std::numeric_limits<int32_t>::min(),
};

enum class ChannelState { None, Initial, Playing, Paused, Stopped };

class Sound;

class AudioChannel final
{
public:
    AudioChannel();
    ~AudioChannel();

    Result play();
    Result pause();
    Result stop();

    Result get_state(ChannelState& state) const;

    Result set_playback_position(std::chrono::milliseconds const& pos);
    Result get_playback_position(std::chrono::milliseconds& pos) const;

    Result set_min_distance(float distance);
    Result set_max_distance(float distance);

    Result set_position_3d(std::array<float, 3> const& position);
    Result set_velocity_3d(std::array<float, 3> const& velocity);
    Result set_direction_3d(std::array<float, 3> const& orientation);

    Result set_volume(float volume_level);
    Result get_volume(float& volume_level);

    Result set_pitch(float pitch_level);
    Result get_pitch(float& pitch_level);

    Result set_looping(bool flag);

private:
    friend class AudioBackend;
    friend class AudioContext;

    Result attach_sound(std::shared_ptr<Sound> const& sound);
    Result unbind_sound();

    void internal_update();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm
