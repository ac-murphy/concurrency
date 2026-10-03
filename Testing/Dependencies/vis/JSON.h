#pragma once
#include "json.hpp"

namespace json_io
{
    inline nlohmann::json load(const std::filesystem::path& path)
    {
        nlohmann::json data;
        std::ifstream data_stream(path);
        data_stream >> data;
        return data;
    }

    inline void write(const std::filesystem::path& path, const nlohmann::json& data)
    {
        std::ofstream data_stream(path);
        data_stream << data;
    }
}
