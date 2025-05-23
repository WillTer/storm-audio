#pragma once

#include <string>

#include <AL/al.h>
#include <AL/alc.h>
#include <SDL_log.h>

#define AL_TRACE_ERRORS() al_trace_errors(__FILE__, __LINE__, __func__)
#define ALC_TRACE_ERRORS(device) alc_trace_errors(__FILE__, __LINE__, __func__, device)

inline void al_trace_errors(std::string const& filename, int line, std::string const& function)
{
    if (auto err = alGetError(); err != AL_NO_ERROR) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[%s:%d][%s] AL Error, code: %x", filename.c_str(), line, function.c_str(), err);
    }
}

inline void alc_trace_errors(std::string const& filename, int line, std::string const& function, ALCdevice* device)
{
    if (auto err = alcGetError(device); err != ALC_NO_ERROR) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[%s:%d][%s] ALC Error, code: %x", filename.c_str(), line, function.c_str(), err);
    }
}
