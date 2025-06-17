
#include <algorithm>
#include <atomic>
#include <cmath>
#include <optional>
#include <semaphore>
#include <variant>

#include <storm_audio/sound.h>
#include <storm_audio/sound_stream.h>
#include <storm_audio/source.h>

#include "al_utils.h"
#include "trace_helpers.h"

using namespace storm::audio;

namespace
{

using SoundPtr       = std::shared_ptr<Sound>;
using SoundStreamPtr = std::shared_ptr<SoundStream>;

struct Fader {
    float                     start_volume;
    float                     end_volume;
    std::chrono::milliseconds duration;
    std::chrono::milliseconds elapsed;

    // For pause and stop after fade
    SourceState target_state;
};

}  // namespace

struct Source::Impl {
    Impl(TraceFunction const& trace_func)
        : m_trace_func {trace_func}
        , m_is_looping {false}
        , m_is_sound_attached {false}
        , m_fader_lock {1}
        , m_sound_lock {1}
    {
        alGenSources(1, &m_source);
        AL_TRACE_ERRORS(m_trace_func);
    }

    ~Impl()
    {
        alSourceStop(m_source);
        AL_TRACE_ERRORS(m_trace_func);

        // Unbind all buffers from source
        detach_sound();

        alDeleteSources(1, &m_source);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void play()
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        alSourcePlay(m_source);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void pause()
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        alSourcePause(m_source);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void stop()
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        alSourceStop(m_source);
        AL_TRACE_ERRORS(m_trace_func);

        detach_sound();
    }

    void play_with_fade(float start_volume, float end_volume, std::chrono::milliseconds const& fade_duration)
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        set_volume(start_volume);

        m_fader_lock.acquire();
        m_fader = Fader {
            .start_volume = start_volume,
            .end_volume   = end_volume,
            .duration     = fade_duration,
            .elapsed      = {},
            .target_state = SourceState::Playing,
        };
        m_fader_lock.release();

        play();
    }

    void pause_with_fade(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {})
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        set_volume(start_volume);

        m_fader_lock.acquire();
        m_fader = Fader {
            .start_volume = start_volume,
            .end_volume   = end_volume,
            .duration     = fade_duration,
            .elapsed      = {},
            .target_state = SourceState::Paused,
        };
        m_fader_lock.release();
    }

    void stop_with_fade(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {})
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        set_volume(start_volume);

        m_fader_lock.acquire();
        m_fader = Fader {
            .start_volume = start_volume,
            .end_volume   = end_volume,
            .duration     = fade_duration,
            .elapsed      = {},
            .target_state = SourceState::Stopped,
        };
        m_fader_lock.release();
    }

    SourceState get_state() const
    {
        if (!m_is_sound_attached.load()) { return SourceState::Free; }

        ALint al_state = 0;
        alGetSourcei(m_source, AL_SOURCE_STATE, &al_state);
        AL_TRACE_ERRORS(m_trace_func);

        switch (al_state) {
        case AL_INITIAL: [[fallthrough]];
        case AL_PAUSED: return SourceState::Paused;
        case AL_PLAYING: return SourceState::Playing;
        case AL_STOPPED: return SourceState::Stopped;
        default: return SourceState::Free;
        }
    }

    void set_playback_position(std::chrono::milliseconds const& pos)
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return;
        }

        if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            m_sound_lock.acquire();
            std::get<SoundStreamPtr>(m_sound)->set_stream_buffer_position(pos);
            m_sound_lock.release();
        } else {
            alSourcef(m_source, AL_SEC_OFFSET, pos.count() / 1000.0F);
            AL_TRACE_ERRORS(m_trace_func);
        }
    }

    std::chrono::milliseconds get_playback_position()
    {
        if (!m_is_sound_attached.load()) {
            TRACE_WARN(m_trace_func, "Source is empty");
            return {};
        }

        auto start_ms = std::chrono::milliseconds(0);
        if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            m_sound_lock.acquire();
            start_ms = std::get<SoundStreamPtr>(m_sound)->get_stream_buffer_position();
            m_sound_lock.release();
        }

        ALfloat offset_s = 0;
        alGetSourcef(m_source, AL_SEC_OFFSET, &offset_s);
        AL_TRACE_ERRORS(m_trace_func);

        return start_ms + std::chrono::milliseconds(static_cast<uint64_t>(offset_s * 1000));
    }

    void set_min_distance(float distance) const
    {
        alSourcef(m_source, AL_REFERENCE_DISTANCE, distance);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_max_distance(float distance) const
    {
        alSourcef(m_source, AL_MAX_DISTANCE, distance);
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_position_3d(std::array<float, 3> const& position) const
    {
        alSourcefv(m_source, AL_POSITION, position.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_velocity_3d(std::array<float, 3> const& velocity) const
    {
        alSourcefv(m_source, AL_VELOCITY, velocity.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_direction_3d(std::array<float, 3> const& direction) const
    {
        alSourcefv(m_source, AL_DIRECTION, direction.data());
        AL_TRACE_ERRORS(m_trace_func);
    }

    void set_rolloff(float factor)
    {
        alSourcef(m_source, AL_ROLLOFF_FACTOR, factor);
        AL_TRACE_ERRORS(m_trace_func);
    }

    float get_rolloff() const
    {
        float factor = 0.0F;
        alGetSourcef(m_source, AL_ROLLOFF_FACTOR, &factor);
        AL_TRACE_ERRORS(m_trace_func);

        return factor;
    }

    void set_volume(float volume_level) const
    {
        alSourcef(m_source, AL_GAIN, std::max(volume_level, 0.0F));
        AL_TRACE_ERRORS(m_trace_func);
    }

    float get_volume() const
    {
        float volume_level = 0.0F;
        alGetSourcef(m_source, AL_GAIN, &volume_level);
        AL_TRACE_ERRORS(m_trace_func);

        return volume_level;
    }

    void set_volume_min(float volume_min)
    {
        alSourcef(m_source, AL_MIN_GAIN, std::clamp(volume_min, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_trace_func);
    }

    float get_volume_min() const
    {
        float volume_min = 0.0F;
        alGetSourcef(m_source, AL_MIN_GAIN, &volume_min);
        AL_TRACE_ERRORS(m_trace_func);

        return volume_min;
    }

    void set_volume_max(float volume_max)
    {
        alSourcef(m_source, AL_MAX_GAIN, std::clamp(volume_max, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_trace_func);
    }

    float get_volume_max() const
    {
        float volume_max = 0.0F;
        alGetSourcef(m_source, AL_MAX_GAIN, &volume_max);
        AL_TRACE_ERRORS(m_trace_func);

        return volume_max;
    }

    void set_pitch(float pitch_level) const
    {
        alSourcef(m_source, AL_PITCH, std::clamp(pitch_level, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_trace_func);
    }

    float get_pitch() const
    {
        float pitch_level = 0.0F;
        alGetSourcef(m_source, AL_PITCH, &pitch_level);
        AL_TRACE_ERRORS(m_trace_func);

        return pitch_level;
    }

    void set_looping(bool flag)
    {
        m_is_looping = flag;

        m_sound_lock.acquire();
        if (std::holds_alternative<SoundPtr>(m_sound)) {
            alSourcei(m_source, AL_LOOPING, m_is_looping ? AL_TRUE : AL_FALSE);
            AL_TRACE_ERRORS(m_trace_func);
        }
        m_sound_lock.release();
    }

    void reset_source_parameters(bool is_stereo)
    {
        m_is_looping = false;

        m_fader_lock.acquire();
        m_fader = std::nullopt;
        m_fader_lock.release();

        alSourcei(m_source, AL_LOOPING, AL_FALSE);
        AL_TRACE_ERRORS(m_trace_func);

        alSourcei(m_source, AL_SOURCE_RELATIVE, is_stereo ? AL_TRUE : AL_FALSE);
        AL_TRACE_ERRORS(m_trace_func);

        alSourcei(m_source, AL_ROLLOFF_FACTOR, is_stereo ? 0 : 1);
        AL_TRACE_ERRORS(m_trace_func);

        // Reset 3D data
        set_position_3d({0, 0, -1});
        set_direction_3d({});
        set_velocity_3d({});
    }

    template <typename T>
    void attach(T const& sound)
    {
        m_sound_lock.acquire();

        this->m_sound = sound;
        sound->attach_source(m_source);
        m_is_sound_attached.store(true);
        reset_source_parameters(is_flag_enabled(sound->get_flags(), Sound::Flags::Stereo2D));

        m_sound_lock.release();
    }

    void detach_sound()
    {
        alSourcei(m_source, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_trace_func);

        m_sound_lock.acquire();

        if (std::holds_alternative<SoundPtr>(m_sound)) {
            std::get<SoundPtr>(m_sound)->detach_source(m_source);
        } else if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            std::get<SoundStreamPtr>(m_sound)->detach_source();
        }

        m_sound = nullptr;
        m_is_sound_attached.store(false);

        m_sound_lock.release();
    }

    void update(std::chrono::milliseconds const& elapsed)
    {
        if (!m_is_sound_attached.load()) { return; }

        auto const state = get_state();
        if (state == SourceState::Paused) { return; }

        process_fader(elapsed);

        if (!m_sound_lock.try_acquire()) { return; }  // Do not lock on update
        if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            auto const updated = std::get<SoundStreamPtr>(m_sound)->update(m_is_looping);
            m_sound_lock.release();

            if (state == SourceState::Playing) { return; }

            // If sound is existing, and we have data for it, then it wasn't stopped by a program
            // but by a lack of buffer data (because of lag or something like that), so continue to play it
            if (updated > 0) {
                play();
            } else {
                detach_sound();
            }
        } else if (state == SourceState::Stopped) {
            m_sound_lock.release();
            detach_sound();
        } else {
            m_sound_lock.release();
        }
    }

    void process_fader(std::chrono::milliseconds const& elapsed)
    {
        m_fader_lock.acquire();

        if (!m_fader.has_value()) {
            m_fader_lock.release();
            return;
        }

        auto& fader = m_fader.value();
        fader.elapsed += elapsed;

        if (fader.duration > fader.elapsed) {
            auto const  t      = static_cast<float>(fader.elapsed.count()) / fader.duration.count();
            float const volume = std::lerp(fader.start_volume, fader.end_volume, t);
            set_volume(volume);
        } else {
            set_volume(fader.end_volume);

            switch (fader.target_state) {
            case SourceState::Paused: pause(); break;
            case SourceState::Stopped: stop(); break;
            default: break;
            }

            m_fader = std::nullopt;
        }

        m_fader_lock.release();
    }

    TraceFunction m_trace_func;

    unsigned m_source;
    bool     m_is_looping;

    std::variant<std::nullptr_t, SoundPtr, SoundStreamPtr> m_sound;

    std::optional<Fader> m_fader;

    std::atomic_bool      m_is_sound_attached;
    std::binary_semaphore m_fader_lock;
    std::binary_semaphore m_sound_lock;
};

Source::Source(TraceFunction const& trace_func) : m_impl {std::make_unique<Impl>(trace_func)} {}
Source::~Source() = default;

void Source::play()
{
    m_impl->play();
}

void Source::pause()
{
    m_impl->pause();
}

void Source::stop()
{
    m_impl->stop();
}

void Source::play_with_fade(
    float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->play_with_fade(start_volume, end_volume, fade_duration);
}

void Source::pause_with_fade(
    float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->pause_with_fade(start_volume, end_volume, fade_duration);
}

void Source::stop_with_fade(
    float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->stop_with_fade(start_volume, end_volume, fade_duration);
}

SourceState Source::get_state() const
{
    return m_impl->get_state();
}

void Source::set_playback_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_playback_position(pos);
}

std::chrono::milliseconds Source::get_playback_position() const
{
    return m_impl->get_playback_position();
}

void Source::set_min_distance(float distance)
{
    m_impl->set_min_distance(distance);
}

void Source::set_max_distance(float distance)
{
    m_impl->set_max_distance(distance);
}

void Source::set_position_3d(std::array<float, 3> const& position)
{
    m_impl->set_position_3d(position);
}

void Source::set_velocity_3d(std::array<float, 3> const& velocity)
{
    m_impl->set_velocity_3d(velocity);
}

void Source::set_direction_3d(std::array<float, 3> const& direction)
{
    m_impl->set_direction_3d(direction);
}

void Source::set_rolloff(float factor)
{
    m_impl->set_rolloff(factor);
}

float Source::get_rolloff() const
{
    return m_impl->get_rolloff();
}

void Source::set_volume(float volume_level)
{
    m_impl->set_volume(volume_level);
}

float Source::get_volume() const
{
    return m_impl->get_volume();
}

void Source::set_volume_min(float volume_min)
{
    m_impl->set_volume_min(volume_min);
}

float Source::get_volume_min() const
{
    return m_impl->get_volume_min();
}

void Source::set_volume_max(float volume_max)
{
    m_impl->set_volume_max(volume_max);
}

float Source::get_volume_max() const
{
    return m_impl->get_volume_max();
}

void Source::set_pitch(float pitch_level)
{
    m_impl->set_pitch(pitch_level);
}

float Source::get_pitch() const
{
    return m_impl->get_pitch();
}

void Source::set_looping(bool flag)
{
    m_impl->set_looping(flag);
}

void Source::attach_sound(std::shared_ptr<Sound> const& sound)
{
    m_impl->attach(sound);
}

void Source::attach_sound_stream(std::shared_ptr<SoundStream> const& sound)
{
    m_impl->attach(sound);
}

void Source::detach_sound()
{
    m_impl->detach_sound();
}

void Source::update(std::chrono::milliseconds const& elapsed)
{
    m_impl->update(elapsed);
}
