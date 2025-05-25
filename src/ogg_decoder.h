#pragma once

#include <memory>

#include <storm_audio/data_stream.h>
#include <storm_audio/debug_tracer.h>

namespace storm::audio
{

class OggDecoder: virtual public DataStream
{
public:
    OggDecoder(std::shared_ptr<DebugTracer> const& tracer, Format format);
    ~OggDecoder() override;

    Result load_file(std::filesystem::path const& file_path) override;

    bool is_valid() override;

    int get_channels() const override;
    int get_sample_rate() const override;

    Format get_data_format() const override;

    size_t get_samples(std::vector<char>& buffer, size_t sample_count) override;
    size_t get_samples_all(std::vector<char>& buffer) override;

    void seek_start() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
