#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace binary_io
{
    template <typename T>
    requires (!std::same_as<T, bool>)
    std::vector<T> read(const std::filesystem::path& path)
    {
        if (!std::filesystem::is_regular_file(path))
        {
            std::cerr << "file does not exist" << std::endl;
            return {};
        }

        const std::streamsize size = std::filesystem::file_size(path);
        constexpr size_t element_size = sizeof(T);
        const auto count = size / element_size;
        if (size % element_size != 0)
        {
            std::cerr << "invalid file size" << std::endl;
            return {};
        }

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            std::cerr << "could not open file" << std::endl;
            return {};
        }

        std::vector<T> data(count);
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    inline std::vector<bool> read_bool(const std::filesystem::path& path)
    {
        const std::vector<uint8_t> unpacked = binary_io::read<uint8_t>(path);
        std::vector<bool> packed(unpacked.size());
        for (auto i = 0; i < unpacked.size(); ++i)
            packed[i] = unpacked[i] == 1;

        return packed;
    }

    template <typename T>
    requires (!std::same_as<T, bool>)
    void write_T(const std::filesystem::path& path, const std::vector<T>& data)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
        file.flush();
        file.close();
    }

    inline void write_bool(const std::filesystem::path& path, const std::vector<bool>& data)
    {
        std::vector<uint8_t> unpacked_bool_list;
        unpacked_bool_list.reserve(data.size());
        for (auto i = 0; i < static_cast<int32_t>(data.size()); ++i) unpacked_bool_list.push_back(data[i]);

        write_T<uint8_t>(path, unpacked_bool_list);
    }
}