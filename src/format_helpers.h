#pragma once

#include <AL/al.h>
#include <AL/alext.h>
#include <storm_audio/data_stream.h>

namespace storm
{

constexpr ALenum convert_to_al_format(IDataStream::Format const format, int const channels)
{
    switch (format) {
    case IDataStream::Format::Unknown: return 0; break;

    // Base formats
    case IDataStream::Format::Int8: return channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8; break;
    case IDataStream::Format::Int16: return channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16; break;

    // Extension
    case IDataStream::Format::Float32: return channels == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32; break;
    }

    return 0;
}

constexpr size_t get_format_sample_size(IDataStream::Format const format)
{
    switch (format) {
    case IDataStream::Format::Unknown: return 0;
    case IDataStream::Format::Int8: return sizeof(int8_t);
    case IDataStream::Format::Int16: return sizeof(int16_t);
    case IDataStream::Format::Float32: return sizeof(float);
    }

    return 0;
}

}  // namespace storm
