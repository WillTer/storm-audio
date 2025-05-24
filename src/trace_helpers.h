#pragma once

#include <storm_audio/debug_tracer.h>

#define TRACE_MESSAGE(tracer, severity, message) \
    do { \
        if (tracer) { (tracer)->trace_message(severity, message, __FILE__, __LINE__, __func__); } \
    } while (false)

#define TRACE_DEBUG(tracer, message) TRACE_MESSAGE(tracer, storm::audio::DebugTracer::Severity::Trace, message)
#define TRACE_INFO(tracer, message) TRACE_MESSAGE(tracer, storm::audio::DebugTracer::Severity::Info, message)
#define TRACE_WARN(tracer, message) TRACE_MESSAGE(tracer, storm::audio::DebugTracer::Severity::Warn, message)
#define TRACE_ERROR(tracer, message) TRACE_MESSAGE(tracer, storm::audio::DebugTracer::Severity::Error, message)
