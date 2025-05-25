#pragma once

#include <AL/al.h>
#include <AL/alext.h>
#include <storm_audio/data_stream.h>

namespace storm::audio
{

constexpr ALenum convert_to_al_format(DataStream::Format const format, int const channels)
{
    switch (format) {
    case DataStream::Format::Unknown: return 0; break;

    // Base formats
    case DataStream::Format::Int8: return channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8; break;
    case DataStream::Format::Int16: return channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16; break;
    }

    return 0;
}

constexpr size_t get_format_sample_size(DataStream::Format const format)
{
    switch (format) {
    case DataStream::Format::Unknown: return 0;
    case DataStream::Format::Int8: return sizeof(int8_t);
    case DataStream::Format::Int16: return sizeof(int16_t);
    }

    return 0;
}

}  // namespace storm::audio
