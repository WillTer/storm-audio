#pragma once

#include <cstdint>
#include <type_traits>

namespace storm
{

class ISound
{
public:
    enum Flags : uint8_t {
        None      = 0,
        Stream    = 1 << 0,
        Stereo2D  = 1 << 1,
        Spatial3D = 1 << 2,
    };

    virtual ~ISound() = default;

    virtual Flags get_flags() const = 0;
};

template <typename T>
    requires std::is_enum_v<T>
static constexpr bool has_flag(T flags, T flag)
{
    return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(flag)) == static_cast<uint64_t>(flag);
}

}  // namespace storm
