#include "wav_decoder.h"

#include <cstring>
#include <vector>

#include <AL/al.h>

#include "const.h"

using namespace storm;

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

    std::vector<uint8_t> pcm_data;
};

template <typename T>
size_t read_chunk(std::vector<uint8_t> const& mem, size_t const offset, T& chunk)
{
    std::memcpy(&chunk, mem.data() + offset, sizeof(chunk));
    return sizeof(chunk);
}

bool is_id_equals(std::string_view const& id, std::string_view const& expected)
{
    if (expected.size() < 4 || id.size() < 4) { return false; }

    return id[0] == expected[0] && id[1] == expected[1] && id[2] == expected[2] && id[3] == expected[3];
}

bool parse_wave_file(std::vector<uint8_t> const& mem, WavFileData& output_data)
{
    size_t               offset     = 0;
    std::vector<uint8_t> audio_data = {};

    RiffChunk riff_chunk = {};
    offset += read_chunk(mem, offset, riff_chunk);
    if (!is_id_equals(riff_chunk.riff, "RIFF") || !is_id_equals(riff_chunk.file_type, "WAVE")) { return false; }

    FormatChunk format_chunk = {};
    offset += read_chunk(mem, offset, format_chunk);
    if (!is_id_equals(format_chunk.id, "fmt ")) { return false; }

    output_data.audio_format    = format_chunk.audio_format;
    output_data.channels        = format_chunk.channels;
    output_data.sample_rate     = format_chunk.sample_rate;
    output_data.bits_per_sample = format_chunk.bits_per_sample;

    while (offset < mem.size()) {
        AnyChunk chunk = {};
        offset += read_chunk(mem, offset, chunk);

        if (is_id_equals(chunk.id, "data") && mem.size() >= (offset + chunk.size)) {
            output_data.pcm_data = std::vector<uint8_t>(mem.data() + offset, mem.data() + offset + chunk.size);

            // No need to read any other chunks
            return true;
        }

        offset += chunk.size;
    }

    return false;
}

}  // namespace

struct WavDecoder::Impl {
    Impl() : m_is_valid {false}, m_channels {0}, m_sample_rate {0}, m_format {Format::Unknown}, m_sample_size {1}, m_offset {0} {}
    ~Impl() = default;

    bool load_memory(std::vector<uint8_t> const& mem)
    {
        WavFileData file_data = {};

        if (!parse_wave_file(mem, file_data)) {
            return false;  // Invalid WAVE-file
        }

        // 1 - PCM, 3 - IEEE Float 32
        if (file_data.audio_format == 1 && file_data.bits_per_sample == 8) {
            m_format = AbstractDataStream::Format::Int8;
        } else if (file_data.audio_format == 1 && file_data.bits_per_sample == 16) {
            m_format = AbstractDataStream::Format::Int16;
        } else if (file_data.audio_format == 3 && file_data.bits_per_sample == 32 && alIsExtensionPresent(FLOAT_EXT_NAME) == AL_TRUE) {
            m_format = AbstractDataStream::Format::Float32;
        } else {
            return false;  // Unsupported format
        }

        m_data        = std::move(file_data.pcm_data);
        m_channels    = file_data.channels;
        m_sample_rate = file_data.sample_rate;
        m_sample_size = file_data.bits_per_sample / 8;

        m_is_valid = true;

        return true;
    }

    size_t get_samples(std::vector<uint8_t>& buffer, size_t sample_count)
    {
        auto const bytes_count = m_data.size() - m_offset;
        if (bytes_count == 0 || sample_count == 0) { return 0; }

        auto const bytes_to_copy = std::min(sample_count * m_sample_size, bytes_count);
        buffer.resize(bytes_to_copy);

        std::memcpy(buffer.data(), m_data.data() + m_offset, buffer.size());
        m_offset += bytes_to_copy;

        return bytes_to_copy;
    }

    size_t get_samples_all(std::vector<uint8_t>& buffer)
    {
        auto const bytes_count = m_data.size() - m_offset;
        if (bytes_count == 0) { return 0; }

        return get_samples(buffer, bytes_count / m_sample_size);
    }

    void seek_start()
    {
        m_offset = 0;
    }

    bool m_is_valid;

    std::vector<uint8_t> m_data;
    int                  m_channels;
    int                  m_sample_rate;
    Format               m_format;

    size_t m_sample_size;
    size_t m_offset;
};

WavDecoder::WavDecoder() : m_impl {std::make_unique<Impl>()} {}

WavDecoder::~WavDecoder() = default;

bool WavDecoder::load_memory(std::vector<uint8_t> const& mem)
{
    return m_impl->load_memory(mem);
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

AbstractDataStream::Format WavDecoder::get_data_format() const
{
    return m_impl->m_format;
}

size_t WavDecoder::get_samples(std::vector<uint8_t>& buffer, size_t sample_count)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples(buffer, sample_count);
}

size_t WavDecoder::get_samples_all(std::vector<uint8_t>& buffer)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples_all(buffer);
}

void WavDecoder::seek_start()
{
    m_impl->seek_start();
}
