#pragma once

#include <string>

#include <AL/al.h>
#include <AL/alc.h>

#define AL_TRACE_ERRORS(trace_func) al_trace_errors(trace_func, __FILE__, __LINE__, __func__);

#define ALC_TRACE_ERRORS(trace_func, device) alc_trace_errors(trace_func, __FILE__, __LINE__, __func__, device);

constexpr std::string get_al_error_text(ALenum error)
{
    switch (error) {
    case AL_INVALID_NAME: return "Invalid name (ID) passed to an AL call";
    case AL_INVALID_ENUM: return "Invalid enumeration passed to AL call";
    case AL_INVALID_VALUE: return "Invalid value passed to AL call";
    case AL_INVALID_OPERATION: return "Illegal AL call";
    case AL_OUT_OF_MEMORY: return "Not enough memory to execute the AL call";
    }

    return "Unknown";
}

constexpr std::string get_alc_error_text(ALCenum error)
{
    switch (error) {
    case ALC_INVALID_DEVICE: return "Invalid device handle";
    case ALC_INVALID_CONTEXT: return "Invalid context handle";
    case ALC_INVALID_ENUM: return "Invalid enumeration passed to an ALC call";
    case ALC_INVALID_VALUE: return "Invalid value passed to an ALC call";
    case ALC_OUT_OF_MEMORY: return "Out of memory";
    }

    return "Unknown";
}

inline void
al_trace_errors(storm::audio::TraceFunction const& trace_func, std::string const& filename, int line, std::string const& function)
{
    if (auto err = alGetError(); err != AL_NO_ERROR) {
        trace_func(storm::audio::MessageSeverity::Warn, "AL error: " + get_al_error_text(err), filename, line, function);
    }
}

inline void alc_trace_errors(
    storm::audio::TraceFunction const& trace_func, std::string const& filename, int line, std::string const& function, ALCdevice* device)
{
    if (auto err = alcGetError(device); err != ALC_NO_ERROR) {
        trace_func(storm::audio::MessageSeverity::Warn, "ALC error: " + get_alc_error_text(err), filename, line, function);
    }
}
