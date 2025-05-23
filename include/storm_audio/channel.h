#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>

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

enum class ChannelState { None, Playing, Paused, Stopped };

class IChannel
{
public:
    virtual ~IChannel() = default;

    virtual Result play()  = 0;
    virtual Result pause() = 0;
    virtual Result stop()  = 0;

    virtual Result get_state(ChannelState& state) const = 0;

    virtual Result set_playback_position(std::chrono::milliseconds const& pos) = 0;
    virtual Result get_playback_position(std::chrono::milliseconds& pos) const = 0;

    virtual Result set_min_distance(float distance) = 0;
    virtual Result set_max_distance(float distance) = 0;

    virtual Result set_position_3d(std::array<float, 3> const& position)     = 0;
    virtual Result set_velocity_3d(std::array<float, 3> const& velocity)     = 0;
    virtual Result set_direction_3d(std::array<float, 3> const& orientation) = 0;

    virtual Result set_volume(float volume_level)  = 0;
    virtual Result get_volume(float& volume_level) = 0;

    virtual Result set_pitch(float pitch_level)  = 0;
    virtual Result get_pitch(float& pitch_level) = 0;

    virtual Result set_looping(bool flag) = 0;
};

}  // namespace storm
