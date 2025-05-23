#pragma once

#include <memory>

#include <base_data_stream.h>

namespace storm
{

class WavDecoder: virtual public BaseDataStream
{
public:
    WavDecoder();
    ~WavDecoder() override;

    bool load_memory(std::vector<uint8_t> const& mem) override;

    bool is_valid() override;

    int get_channels() const override;
    int get_sample_rate() const override;

    Format get_data_format() const override;

    size_t get_samples(std::vector<uint8_t>& buffer, size_t sample_count) override;
    size_t get_samples_all(std::vector<uint8_t>& buffer) override;

    void seek_start() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace storm
