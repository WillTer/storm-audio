#pragma once

#include <storm_audio/trace_func.h>

#define TRACE_MESSAGE(trace_func, severity, message) trace_func(severity, message, __FILE__, __LINE__, __func__);

#define TRACE_DEBUG(trace_func, message) TRACE_MESSAGE(trace_func, storm::audio::MessageSeverity::Trace, message)
#define TRACE_INFO(trace_func, message) TRACE_MESSAGE(trace_func, storm::audio::MessageSeverity::Info, message)
#define TRACE_WARN(trace_func, message) TRACE_MESSAGE(trace_func, storm::audio::MessageSeverity::Warn, message)
#define TRACE_ERROR(trace_func, message) TRACE_MESSAGE(trace_func, storm::audio::MessageSeverity::Error, message)
