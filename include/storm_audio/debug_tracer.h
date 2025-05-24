#pragma once

#include <filesystem>
#include <string>

namespace storm::audio
{

// This is an abstract class that you need to inherit and implement trace_message method
// Then you can pass your object to Backend and this will enable tracing
class DebugTracer
{
public:
    virtual ~DebugTracer() = default;

    enum class Severity { Trace, Info, Warn, Error };

    virtual void trace_message(
        Severity                     severity,
        std::string const&           message,
        std::filesystem::path const& source_file,
        size_t                       line,
        std::string const&           function_name) = 0;
};

}  // namespace storm::audio
