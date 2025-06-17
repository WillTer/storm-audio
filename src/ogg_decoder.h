#pragma once

#include <memory>

#include <storm_audio/data_stream.h>
#include <storm_audio/trace_func.h>

namespace storm::audio
{

class OggDecoder: virtual public DataStream
{
public:
    OggDecoder(TraceFunction const& trace_func, Format format);
    ~OggDecoder() override;

    Result load_file(std::filesystem::path const& file_path) override;

    bool is_valid() override;

    int get_channels() const override;
    int get_sample_rate() const override;

    Format get_data_format() const override;

    size_t get_samples(std::vector<char>& buffer, size_t sample_count) override;
    size_t get_samples_all(std::vector<char>& buffer) override;

    void seek_start() override;

    void                      set_buffer_position(std::chrono::milliseconds const& pos) override;
    std::chrono::milliseconds get_buffer_position() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm::audio
