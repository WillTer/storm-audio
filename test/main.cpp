#include <chrono>
#include <cstdio>
#include <thread>

#include <storm_audio/backend.h>

using namespace storm::audio;

class Tracer: public DebugTracer
{
public:
    void trace_message(
        Severity                     severity,
        std::string const&           message,
        std::filesystem::path const& source_file,
        size_t                       line,
        std::string const&           function_name) override
    {
#define PRINTF_MESSAGE(severity_text) \
    printf("[" #severity_text "][%s:%zd][%s] %s\n", source_file.filename().string().c_str(), line, function_name.c_str(), message.c_str())

        switch (severity) {
        case DebugTracer::Severity::Trace: PRINTF_MESSAGE(TRACE); break;
        case DebugTracer::Severity::Info: PRINTF_MESSAGE(INFO); break;
        case DebugTracer::Severity::Warn: PRINTF_MESSAGE(WARN); break;
        case DebugTracer::Severity::Error: PRINTF_MESSAGE(ERROR); break;
        }
    }
};

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <streamed wav/ogg file 1> [wav/ogg file 2] ... [wav/ogg file n]", argv[0]);
        return -1;
    }

    auto tracer  = std::make_shared<Tracer>();
    auto backend = std::make_unique<Backend>(tracer);

    std::vector<Channel*> channels = {};

    for (int i = 1; i < argc; ++i) {
        auto const flags = i == 1 ? Sound::Flags::Stereo2D | Sound::Flags::Stream : Sound::Flags::Stereo2D;
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
