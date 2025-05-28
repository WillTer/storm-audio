#pragma once

#include <string>

#include <AL/al.h>
#include <AL/alc.h>
#include <storm_audio/debug_tracer.h>

#define AL_TRACE_ERRORS(tracer) \
    do { \
        if (tracer) { al_trace_errors(*(tracer), __FILE__, __LINE__, __func__); } \
    } while (false)

#define ALC_TRACE_ERRORS(tracer, device) \
    do { \
        if (tracer) { alc_trace_errors(*(tracer), __FILE__, __LINE__, __func__, device); } \
    } while (false)

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

inline void al_trace_errors(storm::audio::DebugTracer& tracer, std::string const& filename, int line, std::string const& function)
{
    if (auto err = alGetError(); err != AL_NO_ERROR) {
        __debugbreak();
        tracer.trace_message(storm::audio::DebugTracer::Severity::Warn, "AL error: " + get_al_error_text(err), filename, line, function);
    }
}

inline void
alc_trace_errors(storm::audio::DebugTracer& tracer, std::string const& filename, int line, std::string const& function, ALCdevice* device)
{
    if (auto err = alcGetError(device); err != ALC_NO_ERROR) {
        tracer.trace_message(storm::audio::DebugTracer::Severity::Warn, "ALC error: " + get_alc_error_text(err), filename, line, function);
    }
}
