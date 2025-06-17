#pragma once

#include <memory>
#include <type_traits>

#include "data_stream.h"
#include "enum_flags.h"
#include "trace_func.h"

namespace storm::audio
{

class Sound final
{
public:
    enum class Flags {
        None      = 0,
        Stereo2D  = 1 << 0,
        Spatial3D = 1 << 1,
    };

    Sound(TraceFunction const& trace_func, std::unique_ptr<DataStream> const& stream, Flags flags);
    ~Sound();

    Flags get_flags() const;

private:
    friend class Source;

    void attach_source(unsigned source);
    void detach_source(unsigned source);

    int get_channels() const;

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
