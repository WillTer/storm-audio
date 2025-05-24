#include <chrono>
#include <cstdio>
#include <thread>

#include <storm_audio/audio_backend.h>

using namespace storm;

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <streamed wav file 1> [wav file 2] ... [wav file n]", argv[0]);
        return -1;
    }

    auto                       backend  = std::make_unique<AudioBackend>();
    std::vector<AudioChannel*> channels = {};

    for (int i = 1; i < argc; ++i) {
        auto const flags = static_cast<Sound::Flags>(i == 1 ? Sound::Flags::Stereo2D | Sound::Flags::Stream : Sound::Flags::Stereo2D);
        auto       sound = backend->create_sound(argv[i], flags);

        auto* channel = backend->attach_sound(sound);
        channel->set_looping(true);
        channels.push_back(channel);
    }

    for (auto* channel: channels) {
        channel->play();
    }

    constexpr auto pause = std::chrono::milliseconds(10);
    while (true) {
        backend->update();
        std::this_thread::sleep_for(pause);
    }

    return 0;
}
