
#include <algorithm>
#include <cmath>
#include <optional>
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
    Impl(std::shared_ptr<DebugTracer> const& tracer) : m_tracer {tracer}, m_is_looping {false}
    {
        alGenSources(1, &m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    ~Impl()
    {
        alSourceStop(m_source);
        AL_TRACE_ERRORS(m_tracer);

        // Unbind all buffers from source
        detach_sound();

        alDeleteSources(1, &m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    void play(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {})
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) {
            TRACE_WARN(m_tracer, "Source is empty");
            return;
        }

        if (fade_duration.count() > 0) {
            set_volume(start_volume);

            m_fader = Fader {
                .start_volume = start_volume,
                .end_volume   = end_volume,
                .duration     = fade_duration,
                .elapsed      = {},
                .target_state = SourceState::Playing,
            };
        } else {
            // Set end volume if not fading
            set_volume(end_volume);
        }

        alSourcePlay(m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    void pause(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {})
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) {
            TRACE_WARN(m_tracer, "Source is empty");
            return;
        }

        if (fade_duration.count() > 0) {
            set_volume(start_volume);

            m_fader = Fader {
                .start_volume = start_volume,
                .end_volume   = end_volume,
                .duration     = fade_duration,
                .elapsed      = {},
                .target_state = SourceState::Paused,
            };
        } else {
            alSourcePause(m_source);
            AL_TRACE_ERRORS(m_tracer);
        }
    }

    void stop(float start_volume = 1.0F, float end_volume = 1.0F, std::chrono::milliseconds const& fade_duration = {})
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) {
            TRACE_WARN(m_tracer, "Source is empty");
            return;
        }

        if (fade_duration.count() > 0) {
            set_volume(start_volume);

            m_fader = Fader {
                .start_volume = start_volume,
                .end_volume   = end_volume,
                .duration     = fade_duration,
                .elapsed      = {},
                .target_state = SourceState::Stopped,
            };
        } else {
            alSourceStop(m_source);
            AL_TRACE_ERRORS(m_tracer);

            detach_sound();
        }
    }

    SourceState get_state() const
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) { return SourceState::Free; }

        ALint al_state = 0;
        alGetSourcei(m_source, AL_SOURCE_STATE, &al_state);
        AL_TRACE_ERRORS(m_tracer);

        switch (al_state) {
        case AL_INITIAL: [[fallthrough]];
        case AL_PAUSED: return SourceState::Paused;
        case AL_PLAYING: return SourceState::Playing;
        case AL_STOPPED: return SourceState::Stopped;
        default: return SourceState::Free;
        }
    }

    void set_playback_position(std::chrono::milliseconds const& pos) const
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) {
            TRACE_WARN(m_tracer, "Source is empty");
            return;
        }

        if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            std::get<SoundStreamPtr>(m_sound)->set_stream_buffer_position(pos);
        } else {
            alSourcef(m_source, AL_SEC_OFFSET, pos.count() / 1000.0F);
            AL_TRACE_ERRORS(m_tracer);
        }
    }

    std::chrono::milliseconds get_playback_position() const
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) {
            TRACE_WARN(m_tracer, "Source is empty");
            return {};
        }

        auto start_ms = std::chrono::milliseconds(0);
        if (std::holds_alternative<SoundStreamPtr>(m_sound)) { start_ms = std::get<SoundStreamPtr>(m_sound)->get_stream_buffer_position(); }

        ALfloat offset_s = 0;
        alGetSourcef(m_source, AL_SEC_OFFSET, &offset_s);
        AL_TRACE_ERRORS(m_tracer);

        return start_ms + std::chrono::milliseconds(static_cast<uint64_t>(offset_s * 1000));
    }

    void set_min_distance(float distance) const
    {
        alSourcef(m_source, AL_REFERENCE_DISTANCE, distance);
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_max_distance(float distance) const
    {
        alSourcef(m_source, AL_MAX_DISTANCE, distance);
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_position_3d(std::array<float, 3> const& position) const
    {
        alSourcefv(m_source, AL_POSITION, position.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_velocity_3d(std::array<float, 3> const& velocity) const
    {
        alSourcefv(m_source, AL_VELOCITY, velocity.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_direction_3d(std::array<float, 3> const& direction) const
    {
        alSourcefv(m_source, AL_DIRECTION, direction.data());
        AL_TRACE_ERRORS(m_tracer);
    }

    void set_rolloff(float factor)
    {
        alSourcef(m_source, AL_ROLLOFF_FACTOR, factor);
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_rolloff() const
    {
        float factor = 0.0F;
        alGetSourcef(m_source, AL_ROLLOFF_FACTOR, &factor);
        AL_TRACE_ERRORS(m_tracer);

        return factor;
    }

    void set_volume(float volume_level) const
    {
        alSourcef(m_source, AL_GAIN, std::max(volume_level, 0.0F));
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_volume() const
    {
        float volume_level = 0.0F;
        alGetSourcef(m_source, AL_GAIN, &volume_level);
        AL_TRACE_ERRORS(m_tracer);

        return volume_level;
    }

    void set_volume_min(float volume_min)
    {
        alSourcef(m_source, AL_MIN_GAIN, std::clamp(volume_min, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_volume_min() const
    {
        float volume_min = 0.0F;
        alGetSourcef(m_source, AL_MIN_GAIN, &volume_min);
        AL_TRACE_ERRORS(m_tracer);

        return volume_min;
    }

    void set_volume_max(float volume_max)
    {
        alSourcef(m_source, AL_MAX_GAIN, std::clamp(volume_max, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_volume_max() const
    {
        float volume_max = 0.0F;
        alGetSourcef(m_source, AL_MAX_GAIN, &volume_max);
        AL_TRACE_ERRORS(m_tracer);

        return volume_max;
    }

    void set_pitch(float pitch_level) const
    {
        alSourcef(m_source, AL_PITCH, std::clamp(pitch_level, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_pitch() const
    {
        float pitch_level = 0.0F;
        alGetSourcef(m_source, AL_PITCH, &pitch_level);
        AL_TRACE_ERRORS(m_tracer);

        return pitch_level;
    }

    void set_looping(bool flag)
    {
        m_is_looping = flag;

        if (std::holds_alternative<SoundPtr>(m_sound)) {
            alSourcei(m_source, AL_LOOPING, m_is_looping ? AL_TRUE : AL_FALSE);
            AL_TRACE_ERRORS(m_tracer);
        }
    }

    void reset_source_parameters(bool is_stereo)
    {
        m_is_looping = false;
        m_fader      = std::nullopt;

        alSourcei(m_source, AL_LOOPING, AL_FALSE);
        AL_TRACE_ERRORS(m_tracer);

        alSourcei(m_source, AL_SOURCE_RELATIVE, is_stereo ? AL_TRUE : AL_FALSE);
        AL_TRACE_ERRORS(m_tracer);

        alSourcei(m_source, AL_ROLLOFF_FACTOR, is_stereo ? 0 : 1);
        AL_TRACE_ERRORS(m_tracer);

        // Reset 3D data
        set_position_3d({0, 0, -1});
        set_direction_3d({});
        set_velocity_3d({});
    }

    template <typename T>
    void attach(T const& sound)
    {
        this->m_sound = sound;
        sound->attach_source(m_source);

        reset_source_parameters(is_flag_enabled(sound->get_flags(), Sound::Flags::Stereo2D));
    }

    void detach_sound()
    {
        alSourcei(m_source, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_tracer);

        if (std::holds_alternative<SoundPtr>(m_sound)) {
            std::get<SoundPtr>(m_sound)->detach_source(m_source);
        } else if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            std::get<SoundStreamPtr>(m_sound)->detach_source();
        }

        m_sound = nullptr;
    }

    void update(std::chrono::milliseconds const& elapsed)
    {
        if (std::holds_alternative<std::nullptr_t>(m_sound)) { return; }

        auto const state = get_state();
        if (state == SourceState::Paused) { return; }

        process_fader(elapsed);

        if (std::holds_alternative<SoundStreamPtr>(m_sound)) {
            auto const updated = std::get<SoundStreamPtr>(m_sound)->update(m_is_looping);
            if (state == SourceState::Playing) { return; }

            // If sound is existing and we have data for it, then it's wasn't stopped by a program
            // but by a lack of buffer data (because of lag or something like that), so continue to play it
            if (updated > 0) {
                play();
            } else {
                detach_sound();
            }
        } else if (state == SourceState::Stopped) {
            detach_sound();
        }
    }

    void process_fader(std::chrono::milliseconds const& elapsed)
    {
        if (!m_fader.has_value()) { return; }

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
    }

    std::shared_ptr<DebugTracer> m_tracer;

    unsigned m_source;
    bool     m_is_looping;

    std::variant<std::nullptr_t, SoundPtr, SoundStreamPtr> m_sound;

    std::optional<Fader> m_fader;
};

Source::Source(std::shared_ptr<DebugTracer> const& tracer) : m_impl {std::make_unique<Impl>(tracer)} {}
Source::~Source() = default;

void Source::play(float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->play(start_volume, end_volume, fade_duration);
}

void Source::pause(float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->pause(start_volume, end_volume, fade_duration);
}

void Source::stop(float start_volume /*= 1.0F*/, float end_volume /*= 1.0F*/, std::chrono::milliseconds const& fade_duration /*= {}*/)
{
    m_impl->stop(start_volume, end_volume, fade_duration);
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
