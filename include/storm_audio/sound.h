#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>

#include "abstract_data_stream.h"
#include "debug_tracer.h"

namespace storm::audio
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

    Sound(std::shared_ptr<DebugTracer> const& tracer, std::unique_ptr<AbstractDataStream>&& stream, Flags flags);
    ~Sound();

    Flags get_flags() const;

private:
    friend class Channel;

    void attach_buffers(unsigned source);
    void detach_buffers(unsigned source);

    int get_channels() const;

    bool update_buffer(unsigned buffer, bool is_looping);
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

}  // namespace storm::audio
