#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>

#include <complex.h>

#include "abstract_data_stream.h"

namespace storm
{

class Sound final
{
public:
    enum Flags : uint8_t {
        None      = 0,
        Stream    = 1 << 0,
        Stereo2D  = 1 << 1,
        Spatial3D = 1 << 2,
    };

    Sound(std::unique_ptr<AbstractDataStream>&& stream, Flags flags);
    ~Sound();

    Flags get_flags() const;

private:
    friend class AudioChannel;

    void bind_buffers_to_source(unsigned source);
    void unbind_source(unsigned source);

    int get_channels() const;

    bool push_next_data(unsigned buffer, bool is_looping) const;
    void reset_buffers();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

template <typename T>
    requires std::is_enum_v<T>
static constexpr bool has_flag(T flags, T flag)
{
    return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(flag)) == static_cast<uint64_t>(flag);
}

}  // namespace storm
