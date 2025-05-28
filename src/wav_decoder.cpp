#include "wav_decoder.h"

#include <cstring>
#include <vector>

#include <AL/al.h>

using namespace storm::audio;

namespace
{

struct RiffChunk {
    char     riff[4];  // "RIFF"
    uint32_t file_size;
    char     file_type[4];  // Should be "WAVE" for wav-files
};

struct FormatChunk {
    char     id[4];             // "fmt "
    uint32_t size;              // Size of format chunk
    uint16_t audio_format;      // 1 for PCM
    uint16_t channels;          //
    uint32_t sample_rate;       //
    uint32_t bytes_per_second;  // (sample_rate * channels * bit_per_sample) / 8
    uint16_t block_align;       // Size of one sample (including all channels)
    uint16_t bits_per_sample;   // Number of bits represents each sample
};

struct AnyChunk {
    char     id[4];
    uint32_t size;
};

struct WavFileData {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;

    std::vector<char> pcm_data;
};

template <typename T>
size_t read_chunk(std::vector<char> const& mem, size_t const offset, T& chunk)
{
    std::memcpy(&chunk, mem.data() + offset, sizeof(chunk));
    return sizeof(chunk);
}

bool is_id_equals(std::string_view const& id, std::string_view const& expected)
{
    if (expected.size() < 4 || id.size() < 4) { return false; }

    return id[0] == expected[0] && id[1] == expected[1] && id[2] == expected[2] && id[3] == expected[3];
}

bool parse_wave_file(std::vector<char> const& mem, WavFileData& output_data)
{
    size_t            offset     = 0;
    std::vector<char> audio_data = {};

    RiffChunk riff_chunk = {};
    offset += read_chunk(mem, offset, riff_chunk);
    if (!is_id_equals(riff_chunk.riff, "RIFF") || !is_id_equals(riff_chunk.file_type, "WAVE")) { return false; }

    FormatChunk format_chunk = {};
    offset += read_chunk(mem, offset, format_chunk);
    if (!is_id_equals(format_chunk.id, "fmt ")) { return false; }

    constexpr size_t format_expected_size = sizeof(FormatChunk) - sizeof(format_chunk.id) - sizeof(format_chunk.size);
    if (format_chunk.size > format_expected_size) {
        // Skip unnecessary information
        offset += format_chunk.size - format_expected_size;
    }

    output_data.audio_format    = format_chunk.audio_format;
    output_data.channels        = format_chunk.channels;
    output_data.sample_rate     = format_chunk.sample_rate;
    output_data.bits_per_sample = format_chunk.bits_per_sample;

    while (offset < mem.size()) {
        AnyChunk chunk = {};
        offset += read_chunk(mem, offset, chunk);

        if (is_id_equals(chunk.id, "data") && mem.size() >= (offset + chunk.size)) {
            output_data.pcm_data = std::vector<char>(mem.data() + offset, mem.data() + offset + chunk.size);

            // No need to read any other chunks
            return true;
        }

        offset += chunk.size;
    }

    return false;
}

}  // namespace

struct WavDecoder::Impl {
    Impl()
        : m_is_valid {false}
        , m_buffer_pos {std::chrono::milliseconds(0)}
        , m_channels {0}
        , m_sample_rate {0}
        , m_format {Format::Unknown}
        , m_sample_size {1}
        , m_offset {0}
    {
    }

    ~Impl() = default;

    Result load_file(std::filesystem::path const& file_path)
    {
        if (!std::filesystem::exists(file_path)) { return Result::ErrFileNotFound; }

        auto const file = std::shared_ptr<std::FILE>(std::fopen(file_path.string().c_str(), "rb"), [](std::FILE* p) { std::fclose(p); });
        if (!file) { return Result::ErrFileOpenFailed; }

        std::fseek(file.get(), 0, SEEK_END);
        size_t const sz = std::ftell(file.get());
        std::fseek(file.get(), 0, SEEK_SET);

        std::vector<char> file_data = {};
        file_data.resize(sz);
        if (std::fread(file_data.data(), sizeof(char), sz, file.get()) != sz) { return Result::ErrFileOpenFailed; }

        WavFileData wav_data = {};
        if (!parse_wave_file(file_data, wav_data)) {
            return Result::ErrFileFormatInvalid;  // Invalid WAVE-file
        }

        // 1 - PCM
        if (wav_data.audio_format == 1 && wav_data.bits_per_sample == 8) {
            m_format = DataStream::Format::Int8;
        } else if (wav_data.audio_format == 1 && wav_data.bits_per_sample == 16) {
            m_format = DataStream::Format::Int16;
        } else {
            return Result::ErrFileFormatNotSupported;  // Unsupported format
        }

        m_data        = std::move(wav_data.pcm_data);
        m_channels    = wav_data.channels;
        m_sample_rate = wav_data.sample_rate;
        m_sample_size = wav_data.bits_per_sample / 8;

        m_is_valid = true;

        return Result::Ok;
    }

    size_t get_samples(std::vector<char>& buffer, size_t sample_count)
    {
        update_buffer_position();

        auto const bytes_count = m_data.size() - m_offset;
        if (bytes_count == 0 || sample_count == 0) { return 0; }

        auto const bytes_to_copy = std::min(sample_count * m_sample_size * m_channels, bytes_count);
        buffer.resize(bytes_to_copy);

        std::memcpy(buffer.data(), m_data.data() + m_offset, buffer.size());
        m_offset += bytes_to_copy;

        return bytes_to_copy / (m_sample_size * m_channels);
    }

    size_t get_samples_all(std::vector<char>& buffer)
    {
        update_buffer_position();

        auto const bytes_count = m_data.size() - m_offset;
        if (bytes_count == 0) { return 0; }

        return get_samples(buffer, bytes_count / (m_sample_size * m_channels));
    }

    void seek_start()
    {
        m_offset = 0;
    }

    void set_buffer_position(std::chrono::milliseconds const& pos)
    {
        auto const sample_rate_ms = m_sample_rate / 1000;
        auto const sample_offset  = pos.count() * sample_rate_ms;
        m_offset                  = std::min(sample_offset * m_sample_size, m_data.size());
        update_buffer_position();
    }

    std::chrono::milliseconds get_buffer_position() const
    {
        return m_buffer_pos;
    }

    void update_buffer_position()
    {
        auto const sample_offset  = m_offset / m_sample_size;
        auto const sample_rate_ms = m_sample_rate / 1000;
        m_buffer_pos              = std::chrono::milliseconds(sample_offset / sample_rate_ms);
    }

    bool m_is_valid;

    std::chrono::milliseconds m_buffer_pos;

    std::vector<char> m_data;
    int               m_channels;
    int               m_sample_rate;
    Format            m_format;

    size_t m_sample_size;
    size_t m_offset;
};

WavDecoder::WavDecoder() : m_impl {std::make_unique<Impl>()} {}

WavDecoder::~WavDecoder() = default;

Result WavDecoder::load_file(std::filesystem::path const& file_path)
{
    return m_impl->load_file(file_path);
}

bool WavDecoder::is_valid()
{
    return m_impl->m_is_valid;
}

int WavDecoder::get_channels() const
{
    return m_impl->m_channels;
}

int WavDecoder::get_sample_rate() const
{
    return m_impl->m_sample_rate;
}

DataStream::Format WavDecoder::get_data_format() const
{
    return m_impl->m_format;
}

size_t WavDecoder::get_samples(std::vector<char>& buffer, size_t sample_count)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples(buffer, sample_count);
}

size_t WavDecoder::get_samples_all(std::vector<char>& buffer)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples_all(buffer);
}

void WavDecoder::seek_start()
{
    m_impl->seek_start();
}

void WavDecoder::set_buffer_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_buffer_position(pos);
}

std::chrono::milliseconds WavDecoder::get_buffer_position() const
{
    return m_impl->get_buffer_position();
}
