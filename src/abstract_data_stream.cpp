#include <cstdio>
#include <vector>

#include <storm_audio/abstract_data_stream.h>

using namespace storm;

bool AbstractDataStream::load_file(std::filesystem::path const& file_path)
{
    auto const file = std::shared_ptr<std::FILE>(std::fopen(file_path.string().c_str(), "rb"), [](std::FILE* p) { std::fclose(p); });
    if (!file) { return false; }

    std::fseek(file.get(), 0, SEEK_END);
    size_t const sz = std::ftell(file.get());
    std::fseek(file.get(), 0, SEEK_SET);

    std::vector<uint8_t> file_data = {};
    file_data.resize(sz);
    if (std::fread(file_data.data(), sizeof(uint8_t), sz, file.get()) != sz) { return false; }

    return load_memory(file_data);
}
