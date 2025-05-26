
#include <algorithm>

#include <storm_audio/channel.h>
#include <storm_audio/sound.h>

#include "al_utils.h"
#include "trace_helpers.h"

using namespace storm::audio;

struct Channel::Impl {
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

    void play() const
    {
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return;
        }

        alSourcePlay(m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    void pause() const
    {
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return;
        }

        alSourcePause(m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    void stop() const
    {
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return;
        }

        alSourceStop(m_source);
        AL_TRACE_ERRORS(m_tracer);
    }

    ChannelState get_state() const
    {
        ALint al_state = 0;
        alGetSourcei(m_source, AL_SOURCE_STATE, &al_state);
        AL_TRACE_ERRORS(m_tracer);

        switch (al_state) {
        case AL_PLAYING: return ChannelState::Playing;
        case AL_PAUSED: return ChannelState::Paused;
        case AL_STOPPED: return ChannelState::Stopped;
        case AL_INITIAL: return ChannelState::Initial;
        default: return ChannelState::None;
        }
    }

    void set_playback_position(std::chrono::milliseconds const& pos) const
    {
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return;
        }

        if (is_flag_enabled(m_sound->get_flags(), Sound::Flags::Stream)) {
            m_sound->get_stream().set_current_position(pos);
        } else {
            alSourcei(m_source, AL_SEC_OFFSET, static_cast<ALint>(pos.count() / 1000));
        }
    }

    std::chrono::milliseconds get_playback_position() const
    {
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return {};
        }

        if (is_flag_enabled(m_sound->get_flags(), Sound::Flags::Stream)) { return m_sound->get_stream().get_current_position(); }

        int offset_s = 0;
        alGetSourcei(m_source, AL_SEC_OFFSET, &offset_s);

        return std::chrono::milliseconds(offset_s * 1000);
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

    void set_volume(float volume_level) const
    {
        alSourcef(m_source, AL_GAIN, std::clamp(volume_level, 0.0F, 1.0F));
        AL_TRACE_ERRORS(m_tracer);
    }

    float get_volume() const
    {
        float volume_level = 0.0F;
        alGetSourcef(m_source, AL_GAIN, &volume_level);
        AL_TRACE_ERRORS(m_tracer);

        return volume_level;
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
        if (!m_sound) {
            TRACE_WARN(m_tracer, "Channel is empty");
            return;
        }

        if (is_flag_enabled(m_sound->get_flags(), Sound::Flags::Stream)) {
            alSourcei(m_source, AL_LOOPING, AL_FALSE);
        } else {
            alSourcei(m_source, AL_LOOPING, flag ? AL_TRUE : AL_FALSE);
        }

        AL_TRACE_ERRORS(m_tracer);
    }

    void attach_sound(std::shared_ptr<Sound> const& sound)
    {
        this->m_sound = sound;
        sound->attach_buffers(m_source);

        if (is_flag_enabled(sound->get_flags(), Sound::Flags::Stereo2D) && sound->get_channels() == 1) {
            alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
            AL_TRACE_ERRORS(m_tracer);

            alSourcei(m_source, AL_ROLLOFF_FACTOR, 0);
            AL_TRACE_ERRORS(m_tracer);

            // Reset 3D data
            set_position_3d({0, 0, -1});
            set_direction_3d({});
            set_velocity_3d({});
        } else if (sound->get_channels() == 1) {
            alSourcei(m_source, AL_SOURCE_RELATIVE, AL_FALSE);
            AL_TRACE_ERRORS(m_tracer);

            alSourcei(m_source, AL_ROLLOFF_FACTOR, 1);
            AL_TRACE_ERRORS(m_tracer);
        }
    }

    void detach_sound()
    {
        alSourcei(m_source, AL_BUFFER, 0);
        AL_TRACE_ERRORS(m_tracer);

        if (m_sound) {
            m_sound->detach_buffers(m_source);
            m_sound.reset();
        }
    }

    void internal_update() const
    {
        if (!m_sound) { return; }

        auto const state = get_state();
        if (state == ChannelState::Paused || state == ChannelState::Initial) { return; }

        if (is_flag_enabled(m_sound->get_flags(), Sound::Flags::Stream)) { update_stream(); }
    }

    void update_stream() const
    {
        ALint processed = 0;
        alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
        AL_TRACE_ERRORS(m_tracer);

        for (ALint i = 0; i < processed; ++i) {
            ALuint buffer = 0;
            alSourceUnqueueBuffers(m_source, 1, &buffer);
            AL_TRACE_ERRORS(m_tracer);

            if (m_sound->update_buffer(buffer, m_is_looping)) {
                alSourceQueueBuffers(m_source, 1, &buffer);
                AL_TRACE_ERRORS(m_tracer);
            }
        }
    }

    std::shared_ptr<DebugTracer> m_tracer;

    unsigned m_source;
    bool     m_is_looping;

    std::shared_ptr<Sound> m_sound;
};

Channel::Channel(std::shared_ptr<DebugTracer> const& tracer) : m_impl {std::make_unique<Impl>(tracer)} {}
Channel::~Channel() = default;

void Channel::play()
{
    m_impl->play();
}

void Channel::pause()
{
    m_impl->pause();
}

void Channel::stop()
{
    m_impl->stop();
}

ChannelState Channel::get_state() const
{
    return m_impl->get_state();
}

void Channel::set_playback_position(std::chrono::milliseconds const& pos)
{
    m_impl->set_playback_position(pos);
}

std::chrono::milliseconds Channel::get_playback_position() const
{
    return m_impl->get_playback_position();
}

void Channel::set_min_distance(float distance)
{
    m_impl->set_min_distance(distance);
}

void Channel::set_max_distance(float distance)
{
    m_impl->set_max_distance(distance);
}

void Channel::set_position_3d(std::array<float, 3> const& position)
{
    m_impl->set_position_3d(position);
}

void Channel::set_velocity_3d(std::array<float, 3> const& velocity)
{
    m_impl->set_velocity_3d(velocity);
}

void Channel::set_direction_3d(std::array<float, 3> const& orientation)
{
    m_impl->set_direction_3d(orientation);
}

void Channel::set_volume(float volume_level)
{
    m_impl->set_volume(volume_level);
}

float Channel::get_volume() const
{
    return m_impl->get_volume();
}

void Channel::set_pitch(float pitch_level)
{
    m_impl->set_pitch(pitch_level);
}

float Channel::get_pitch() const
{
    return m_impl->get_pitch();
}

void Channel::set_looping(bool flag)
{
    m_impl->set_looping(flag);
}

void Channel::attach_sound(std::shared_ptr<Sound> const& sound)
{
    m_impl->attach_sound(sound);
}

void Channel::detach_sound()
{
    m_impl->detach_sound();
}

void Channel::internal_update()
{
    m_impl->internal_update();
}
