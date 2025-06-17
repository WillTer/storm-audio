#include "ogg_decoder.h"

#include <cstdio>
#include <cstring>
#include <vector>

#include <vorbis/vorbisfile.h>

#include "format_helpers.h"

using namespace storm::audio;

namespace
{

void trace_decode_errors(TraceFunction const& trace_func, int res)
{
#define TRACE_ERROR_CASE(error) \
    case error: trace_func(storm::audio::MessageSeverity::Error, "Vorbisfile read error: " #error, __FILE__, __LINE__, __func__); break

    switch (res) {
        TRACE_ERROR_CASE(OV_FALSE);
        TRACE_ERROR_CASE(OV_EOF);
        TRACE_ERROR_CASE(OV_HOLE);

        TRACE_ERROR_CASE(OV_EREAD);
        TRACE_ERROR_CASE(OV_EFAULT);
        TRACE_ERROR_CASE(OV_EIMPL);
        TRACE_ERROR_CASE(OV_EINVAL);
        TRACE_ERROR_CASE(OV_ENOTVORBIS);
        TRACE_ERROR_CASE(OV_EBADHEADER);
        TRACE_ERROR_CASE(OV_EVERSION);
        TRACE_ERROR_CASE(OV_ENOTAUDIO);
        TRACE_ERROR_CASE(OV_EBADPACKET);
        TRACE_ERROR_CASE(OV_EBADLINK);
        TRACE_ERROR_CASE(OV_ENOSEEK);
    }
}

}  // namespace

struct OggDecoder::Impl {
    Impl(TraceFunction const& trace_func, Format format)
        : m_trace_func {trace_func}
        , m_is_valid {false}
        , m_buffer_pos {std::chrono::milliseconds(0)}
        , m_channels {0}
        , m_sample_rate {0}
        , m_format {format}
        , m_is_signed {format == Format::Int8 ? 0 : 1}
        , m_sample_size {get_format_sample_size(format)}
        , m_current_section {1}
    {
    }

    ~Impl()
    {
        if (m_is_valid) { ov_clear(&m_stream); }
    }

    Result load_file(std::filesystem::path const& file_path)
    {
        if (!std::filesystem::exists(file_path)) { return Result::ErrFileNotFound; }

        m_file_handler = std::shared_ptr<std::FILE>(std::fopen(file_path.string().c_str(), "rb"), [](std::FILE* p) { std::fclose(p); });
        if (!m_file_handler) { return Result::ErrFileOpenFailed; }

        if (ov_open_callbacks(m_file_handler.get(), &m_stream, nullptr, 0, OV_CALLBACKS_NOCLOSE) < 0) {
            return Result::ErrFileFormatInvalid;
        }

        vorbis_info* info = ov_info(&m_stream, -1);
        m_channels        = info->channels;
        m_sample_rate     = info->rate;

        m_is_valid = true;

        return Result::Ok;
    }

    size_t get_samples(std::vector<char>& buffer, size_t sample_count)
    {
        // Set time position BEFORE reading buffer, as we will get offset inside buffer later from OpenAL
        update_buffer_position();

        auto const bytes_need = sample_count * m_sample_size * m_channels;
        buffer.resize(bytes_need);

        size_t bytes_read = 0;
        while (bytes_read < bytes_need) {
            auto const res = ov_read(
                &m_stream,
                buffer.data() + bytes_read,
                static_cast<int>(bytes_need - bytes_read),
                0,  // 0 - little-endian
                static_cast<int>(m_sample_size),
                m_is_signed,
                &m_current_section);

            // Error occured
            if (res < 0) {
                trace_decode_errors(m_trace_func, res);
                break;
            }

            // EOF
            if (res == 0) { break; }

            bytes_read += res;
        }

        if (bytes_read != bytes_need) {
            buffer.resize(bytes_read);  // Shrink buffer to actual size
        }

        return bytes_read / (m_sample_size * m_channels);
    }

    size_t get_samples_all(std::vector<char>& buffer_out)
    {
        // Set time position BEFORE reading buffer, as we will get offset inside buffer later from OpenAL
        update_buffer_position();

        std::vector<char> buffer        = {};
        size_t            total_samples = 0;
        size_t            samples_read  = 0;

        constexpr size_t pass_sample_count = 4096;

        while ((samples_read = get_samples(buffer, pass_sample_count)) > 0) {
            buffer_out.insert(buffer_out.end(), buffer.begin(), buffer.end());
            total_samples += samples_read;
        }

        return total_samples;
    }

    void seek_start()
    {
        ov_raw_seek(&m_stream, 0);
    }

    void set_buffer_position(std::chrono::milliseconds const& pos)
    {
        double const pos_s = pos.count() / 1000.0;
        ov_time_seek(&m_stream, pos_s);
        update_buffer_position();
    }

    std::chrono::milliseconds get_buffer_position() const
    {
        return m_buffer_pos;
    }

    void update_buffer_position()
    {
        double const pos_ms = ov_time_tell(&m_stream) * 1000.0;
        m_buffer_pos        = std::chrono::milliseconds(static_cast<int64_t>(pos_ms));
    }

    TraceFunction m_trace_func;

    bool m_is_valid;

    std::shared_ptr<std::FILE> m_file_handler;
    mutable OggVorbis_File     m_stream;

    std::chrono::milliseconds m_buffer_pos;

    int    m_channels;
    int    m_sample_rate;
    Format m_format;
    int    m_is_signed;

    size_t m_sample_size;
    int    m_current_section;
};

OggDecoder::OggDecoder(TraceFunction const& trace_func, Format format) : m_impl {std::make_unique<Impl>(trace_func, format)} {}

OggDecoder::~OggDecoder() = default;

Result OggDecoder::load_file(std::filesystem::path const& file_path)
{
    return m_impl->load_file(file_path);
}

bool OggDecoder::is_valid()
{
    return m_impl->m_is_valid;
}

int OggDecoder::get_channels() const
{
    return m_impl->m_channels;
}

int OggDecoder::get_sample_rate() const
{
    return m_impl->m_sample_rate;
}

DataStream::Format OggDecoder::get_data_format() const
{
    return m_impl->m_format;
}

size_t OggDecoder::get_samples(std::vector<char>& buffer, size_t sample_count)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples(buffer, sample_count);
}

size_t OggDecoder::get_samples_all(std::vector<char>& buffer)
{
    if (!m_impl->m_is_valid) { return 0; }

    return m_impl->get_samples_all(buffer);
}

void OggDecoder::seek_start()
{
    m_impl->seek_start();
}

void OggDecoder::set_buffer_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_buffer_position(pos);
}

std::chrono::milliseconds OggDecoder::get_buffer_position() const
{
    return m_impl->get_buffer_position();
}
