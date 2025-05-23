#define SDL_MAIN_HANDLED
#include <chrono>
#include <thread>

#include <SDL2/SDL.h>
#include <storm_audio/audio_backend.h>

using namespace storm;

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can't init SDL: %s", SDL_GetError());
        return -1;
    }

    if (argc == 1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Usage: %s <wav file>", argv[0]);
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

    SDL_Quit();
    return 0;
}
