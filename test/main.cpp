#include <chrono>
#include <cstdio>
#include <thread>

#include <storm_audio/audio_backend.h>

using namespace storm;

int main(int argc, char** argv)
{
    if (argc == 1) {
        printf("Usage: %s <wav file>", argv[0]);
        return -1;
    }

    std::string const file = argv[1];

    {
        auto backend = IAudioBackend::create_backend();
        backend->init();
        auto sound   = backend->create_sound(file, ISound::Flags::Stereo2D);
        auto channel = backend->bind_sound_to_empty_channel(sound);
        {
            auto channel_lock = channel.lock();
            channel_lock->set_looping(true);
            channel_lock->play();
        }

        bool is_running = true;
        while (is_running) {
            backend->update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return 0;
}
