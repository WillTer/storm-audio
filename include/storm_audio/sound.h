#pragma once

#include <memory>
#include <type_traits>

#include "data_stream.h"
#include "debug_tracer.h"
#include "enum_flags.h"

namespace storm::audio
{

class Sound final
{
public:
    enum class Flags {
        None      = 0,
        Stream    = 1 << 0,
        Stereo2D  = 1 << 1,
        Spatial3D = 1 << 2,
    };

    Sound(std::shared_ptr<DebugTracer> const& tracer, std::unique_ptr<DataStream>&& stream, Flags flags);
    ~Sound();

    Flags get_flags() const;

private:
    friend class Source;

    void attach_buffers(unsigned source);
    void detach_buffers(unsigned source);

    int get_channels() const;

    bool update_buffer(unsigned buffer, bool is_looping);
    void reset_buffers();

    DataStream& get_stream() const;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio

namespace type_traits
{

template <>
struct is_flag<storm::audio::Sound::Flags>: std::true_type {
};

}  // namespace type_traits
