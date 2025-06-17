#pragma once

#include <memory>
#include <type_traits>

#include "data_stream.h"
#include "sound.h"
#include "trace_func.h"

namespace storm::audio
{

class SoundStream final
{
public:
    SoundStream(
        TraceFunction const&          trace_func,
        std::unique_ptr<DataStream>&& stream,
        Sound::Flags                  flags,
        size_t                        stream_buffer_count,
        size_t                        buffer_sample_count);
    ~SoundStream();

    Sound::Flags get_flags() const;

private:
    friend class Source;

    void attach_source(unsigned source);
    void detach_source();

    int get_channels() const;

    size_t update(bool is_looping);

    void                      set_stream_buffer_position(std::chrono::milliseconds const& pos);
    std::chrono::milliseconds get_stream_buffer_position() const;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
