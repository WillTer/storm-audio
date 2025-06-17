#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace storm::audio
{

enum class MessageSeverity { Trace, Info, Warn, Error };
using TraceFunction = std::function<void(
    MessageSeverity              severity,
    std::string const&           message,
    std::filesystem::path const& source_file,
    size_t                       line,
    std::string const&           function_name)>;

}  // namespace storm::audio
