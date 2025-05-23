#pragma once

#include <storm_audio/data_stream.h>

namespace storm
{

class BaseDataStream: virtual public IDataStream
{
public:
    BaseDataStream();
    ~BaseDataStream() override;

    bool load_file(std::filesystem::path const& file_path) override;
};

}  // namespace storm
