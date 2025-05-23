#pragma once

#include <AL/al.h>
#include <AL/alext.h>
#include <storm_audio/data_stream.h>

namespace storm
{

constexpr IDataStream::Format get_compatible_format(uint16_t bits_per_sample)
{
    switch (bits_per_sample) {
    case 8: return IDataStream::Format::UInt8;
    case 16: return IDataStream::Format::Int16;
    case 32: return IDataStream::Format::Float32;
    }

    return IDataStream::Format::Unknown;
}

constexpr IDataStream::Format convert_format_compatible(IDataStream::Format const format)
{
    switch (format) {
    case IDataStream::Format::Unknown: return IDataStream::Format::Unknown;
    case IDataStream::Format::Int8: [[fallthrough]];
    case IDataStream::Format::UInt8: return IDataStream::Format::UInt8;  // OpenAL always read 8-bit data as unsigned
    case IDataStream::Format::Int16: [[fallthrough]];
    case IDataStream::Format::UInt16: return IDataStream::Format::Int16;  // OpenAL always read 16-bit data as signed
    case IDataStream::Format::Int32: [[fallthrough]];  // OpenAL supports 32-bit data only in float format (and only with extension)
    case IDataStream::Format::Float32:
        if (alIsExtensionPresent("AL_EXT_float32") == AL_TRUE) { return IDataStream::Format::Float32; }
        return IDataStream::Format::Int16;  // Fallback to 16-bit if 32-bit is not supported
    }

    return IDataStream::Format::Unknown;
}

constexpr ALenum convert_to_al_format(IDataStream::Format const format, int const channels)
{
    switch (format) {
    case IDataStream::Format::Unknown: return 0; break;

    // Base formats
    case IDataStream::Format::Int8: [[fallthrough]];
    case IDataStream::Format::UInt8: return channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8; break;
    case IDataStream::Format::Int16: [[fallthrough]];
    case IDataStream::Format::UInt16: return channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16; break;

    // Extension
    case IDataStream::Format::Int32: [[fallthrough]];
    case IDataStream::Format::Float32:
        if (alIsExtensionPresent("AL_EXT_float32") == AL_FALSE) {
            return 0;
        } else {
            return channels == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;
        }
        break;
    }

    return 0;
}

constexpr size_t get_format_sample_size(IDataStream::Format const format)
{
    switch (format) {
    case IDataStream::Format::Unknown: return 0;
    case IDataStream::Format::Int8: return sizeof(int8_t);
    case IDataStream::Format::Int16: return sizeof(int16_t);
    case IDataStream::Format::Int32: return sizeof(int32_t);
    case IDataStream::Format::UInt8: return sizeof(uint8_t);
    case IDataStream::Format::UInt16: return sizeof(uint16_t);
    case IDataStream::Format::Float32: return sizeof(float);
    }

    return 0;
}

}  // namespace storm
