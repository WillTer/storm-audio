#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "result.h"

namespace storm::audio
{

class AbstractDataStream
{
public:
    enum class Format {
        Unknown,
        Int8,
        Int16,
        Float32,
    };

    virtual ~AbstractDataStream() = default;

    virtual Result load_file(std::filesystem::path const& file_path);
    virtual Result load_memory(std::vector<uint8_t> const& mem) = 0;

    virtual bool is_valid() = 0;

    virtual int get_channels() const    = 0;
    virtual int get_sample_rate() const = 0;

    virtual Format get_data_format() const = 0;

    virtual size_t get_samples(std::vector<uint8_t>& buffer, size_t sample_count) = 0;
    virtual size_t get_samples_all(std::vector<uint8_t>& buffer)                  = 0;

    virtual void seek_start() = 0;
};

}  // namespace storm::audio
