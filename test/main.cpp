#include <chrono>
#include <format>
#include <iostream>
#include <thread>

#include <storm_audio/device.h>
#include <storm_audio/source.h>

using namespace storm::audio;

namespace
{

std::atomic_bool m_is_finished = false;

void trace_message(
    MessageSeverity              severity,
    std::string const&           message,
    std::filesystem::path const& source_file,
    size_t                       line,
    std::string const&           function_name)
{
#define PRINTF_MESSAGE(severity_text) \
    std::cout << std::format("[" #severity_text "][{}:{}][{}] {}", source_file.filename().string(), line, function_name, message) \
              << std::endl

    switch (severity) {
    case MessageSeverity::Trace: PRINTF_MESSAGE(TRACE); break;
    case MessageSeverity::Info: PRINTF_MESSAGE(INFO); break;
    case MessageSeverity::Warn: PRINTF_MESSAGE(WARN); break;
    case MessageSeverity::Error: PRINTF_MESSAGE(ERROR); break;
    }
}

void print_usage(std::string_view const& app_path)
{
    std::cout << std::format("Usage: {} <option> <path>", app_path) << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "\t--file\t\tPlay single file as whole" << std::endl;
    std::cout << "\t--file-stream\tPlay single file as stream" << std::endl;
    std::cout << "\t--dir\t\tPlay all files in directory as stream (non-recursive)" << std::endl;
}

void process_input(Source& source)
{
    int  ch   = 0;
    bool done = false;
    while ((ch = getchar()) != 'q' && !done) {
        switch (ch) {
        case 'p':  // Pause/Play
            if (source.get_state() == SourceState::Playing) {
                source.pause();
            } else if (source.get_state() == SourceState::Paused) {
                source.play();
            }
            break;
        case 'n':  // Next
            source.stop();
            done = true;
            break;
        default: break;
        }
    }

    if (ch == 'q') { m_is_finished.store(true); }  // Quit
}

void play_file(Device& device, Sound::Flags flags, std::filesystem::path const& file)
{
    auto const sound  = device.create_sound(file, flags);
    auto const source = device.attach_sound(sound);
    source->set_looping(true);
    source->play();

    process_input(*source);
    m_is_finished.store(true);
}

void play_file_stream(Device& device, Sound::Flags flags, std::filesystem::path const& file)
{
    auto const sound  = device.create_sound_stream(file, flags);
    auto const source = device.attach_sound_stream(sound);
    source->set_looping(true);
    source->play();

    process_input(*source);
    m_is_finished.store(true);
}

void play_dir(Device& device, Sound::Flags flags, std::filesystem::path const& dir)
{
    for (auto const& file: std::filesystem::directory_iterator(dir)) {
        if (file.path().extension() != ".wav" && file.path().extension() != ".ogg") { continue; }

        std::cout << std::format("Now playing: {}", file.path().string()) << std::endl;

        auto const sound  = device.create_sound_stream(file.path(), flags);
        auto const source = device.attach_sound_stream(sound);
        source->set_looping(false);
        source->play_with_fade(0.0F, 1.0F, std::chrono::milliseconds(1000));

        process_input(*source);
        if (m_is_finished.load()) { return; }
    }

    m_is_finished.store(true);
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return -1;
    }

    auto device = std::make_unique<Device>(trace_message);
    auto thread = std::thread([&device]() {
        constexpr auto pause = std::chrono::milliseconds(10);
        while (!m_is_finished.load()) {
            std::this_thread::sleep_for(pause);
            device->update(pause);
        }
    });

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

    thread.join();

    return 0;
}
