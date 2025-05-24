#pragma once

namespace storm::audio
{

enum class Result : uint32_t {
    Ok = 0,
    ErrFileNotFound,
    ErrFileOpenFailed,
    ErrFileFormatInvalid,
    ErrFileFormatNotSupported,

    ErrUnknown,
};

}
