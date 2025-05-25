#pragma once

#include <filesystem>
#include <vector>

#include <storm_audio/result.h>

namespace storm::audio
{

class DataStream
{
public:
    enum class Format {
        Unknown,
        Int8,
        Int16,
    };

    virtual ~DataStream() = default;

    virtual Result load_file(std::filesystem::path const& file_path) = 0;

    virtual bool is_valid() = 0;

    virtual int get_channels() const    = 0;
    virtual int get_sample_rate() const = 0;

    virtual Format get_data_format() const = 0;

    virtual size_t get_samples(std::vector<char>& buffer, size_t sample_count) = 0;
    virtual size_t get_samples_all(std::vector<char>& buffer)                  = 0;

    virtual void seek_start() = 0;
};

}  // namespace storm::audio
