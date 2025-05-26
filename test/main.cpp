#include <chrono>
#include <cstdio>
#include <thread>

#include <storm_audio/backend.h>

using namespace storm::audio;

namespace
{

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

void print_usage(std::string_view const& app_path)
{
    printf("Usage: %s <option> <path>\n", app_path.data());
    printf("Options:\n");
    printf("\t--file\t\tPlay single file as whole\n");
    printf("\t--file-stream\tPlay single file as stream\n");
    printf("\t--dir\t\tPlay all files in directory as stream (non-recursive)\n");
}

void play_file(Backend& backend, Sound::Flags flags, std::filesystem::path const& file)
{
    auto  sound   = backend.create_sound(file, flags);
    auto* channel = backend.attach_sound(sound);
    channel->set_looping(true);
    channel->play();

    constexpr auto pause = std::chrono::milliseconds(10);
    while (true) {
        backend.update();
        std::this_thread::sleep_for(pause);
    }
}

void play_dir(Backend& backend, Sound::Flags flags, std::filesystem::path const& dir)
{
    for (auto const& file: std::filesystem::directory_iterator(dir)) {
        auto  sound   = backend.create_sound(file.path(), flags);
        auto* channel = backend.attach_sound(sound);
        channel->set_looping(false);
        channel->play();

        constexpr auto pause = std::chrono::milliseconds(10);
        while (channel->get_state() != ChannelState::Stopped) {
            backend.update();
            std::this_thread::sleep_for(pause);
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return -1;
    }

    auto tracer  = std::make_shared<Tracer>();
    auto backend = std::make_unique<Backend>(tracer);

    if (std::string_view(argv[1]) == "--file") {
        play_file(*backend, Sound::Flags::Stereo2D, argv[2]);
    } else if (std::string_view(argv[1]) == "--file-stream") {
        play_file(*backend, Sound::Flags::Stereo2D | Sound::Flags::Stream, argv[2]);
    } else if (std::string_view(argv[1]) == "--dir") {
        play_dir(*backend, Sound::Flags::Stereo2D | Sound::Flags::Stream, argv[2]);
    } else {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}
