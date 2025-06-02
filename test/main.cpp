#include <chrono>
#include <cstdio>
#include <thread>

#include <storm_audio/device.h>
#include <storm_audio/source.h>

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

void play_file(Device& device, Sound::Flags flags, std::filesystem::path const& file)
{
    auto const sound  = device.create_sound(file, flags);
    auto const source = device.attach_sound(sound);
    source->set_looping(true);
    source->play();

    constexpr auto pause = std::chrono::milliseconds(10);
    while (true) {
        std::this_thread::sleep_for(pause);
        device.update(pause);
    }
}

void play_file_stream(Device& device, Sound::Flags flags, std::filesystem::path const& file)
{
    auto const sound  = device.create_sound_stream(file, flags);
    auto const source = device.attach_sound_stream(sound);
    source->set_looping(true);
    source->play();

    constexpr auto pause = std::chrono::milliseconds(10);
    while (true) {
        std::this_thread::sleep_for(pause);
        device.update(pause);
    }
}

void play_dir(Device& device, Sound::Flags flags, std::filesystem::path const& dir)
{
    for (auto const& file: std::filesystem::directory_iterator(dir)) {
        auto const sound  = device.create_sound_stream(file.path(), flags);
        auto const source = device.attach_sound_stream(sound);
        source->set_looping(false);
        source->play(0.0F, 1.0F, std::chrono::milliseconds(1000));

        constexpr auto pause = std::chrono::milliseconds(10);
        while (source->get_state() != SourceState::Free) {
            std::this_thread::sleep_for(pause);
            device.update(pause);
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

    auto tracer = std::make_shared<Tracer>();
    auto device = std::make_unique<Device>(tracer);

    if (std::string_view(argv[1]) == "--file") {
        play_file(*device, Sound::Flags::Stereo2D, argv[2]);
    } else if (std::string_view(argv[1]) == "--file-stream") {
        play_file_stream(*device, Sound::Flags::Stereo2D, argv[2]);
    } else if (std::string_view(argv[1]) == "--dir") {
        play_dir(*device, Sound::Flags::Stereo2D, argv[2]);
    } else {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}
