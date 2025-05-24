#pragma once

#include <AL/al.h>
#include <AL/alext.h>
#include <storm_audio/abstract_data_stream.h>

namespace storm::audio
{

constexpr ALenum convert_to_al_format(AbstractDataStream::Format const format, int const channels)
{
    switch (format) {
    case AbstractDataStream::Format::Unknown: return 0; break;

    // Base formats
    case AbstractDataStream::Format::Int8: return channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8; break;
    case AbstractDataStream::Format::Int16: return channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16; break;

    // Extension
    case AbstractDataStream::Format::Float32: return channels == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32; break;
    }

    return 0;
}

constexpr size_t get_format_sample_size(AbstractDataStream::Format const format)
{
    switch (format) {
    case AbstractDataStream::Format::Unknown: return 0;
    case AbstractDataStream::Format::Int8: return sizeof(int8_t);
    case AbstractDataStream::Format::Int16: return sizeof(int16_t);
    case AbstractDataStream::Format::Float32: return sizeof(float);
    }

    return 0;
}

}  // namespace storm::audio
