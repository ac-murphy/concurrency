#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "BIN.h"
#include "JSON.h"

class visualise
{
public:
    visualise() = default;
    ~visualise() = default;

public:
    void init() const
    {
        for (const auto& entry : std::filesystem::directory_iterator(_output_dir))
            std::filesystem::remove_all(entry.path());
    }

    void graph_1d(const std::vector<float>& x_values,
                  const std::vector<float>& y_values)
    {
        std::filesystem::path dir = _output_dir/std::to_string(_op_counter++);
        std::filesystem::create_directories(dir);
        binary_io::write_T(dir/"x_values.bin", x_values);
        binary_io::write_T(dir/"y_values.bin", y_values);

        nlohmann::json metadata;
        metadata["type"] = "graph_1d";
        json_io::write(dir/"metadata.json", metadata);
    }

    void graph_1d_anim(const std::vector<float>& u_values_flattened,
                       const std::vector<float>& t_values,
                       const std::vector<float>& x_values)
    {
        std::filesystem::path dir = _output_dir/std::to_string(_op_counter++);
        std::filesystem::create_directories(dir);
        binary_io::write_T(dir/"u_values.bin", u_values_flattened);
        binary_io::write_T(dir/"t_values.bin", t_values);
        binary_io::write_T(dir/"x_values.bin", x_values);

        nlohmann::json metadata;
        metadata["type"] = "graph_1d_anim";
        json_io::write(dir/"metadata.json", metadata);
    }

    void graph_1d_anim(const std::vector<std::vector<float>>& u_values,
                       const std::vector<float>& t_values,
                       const std::vector<float>& x_values)
    {
        std::vector<float> u_values_flattened;
        for (const auto& row : u_values)
            u_values_flattened.insert(u_values_flattened.end(), row.begin(), row.end());

        visualise::graph_1d_anim(u_values_flattened, t_values, x_values);
    }

    void graph_2d(const std::vector<float>& x_values,
                  const std::vector<float>& y_values,
                  const std::vector<float>& z_values_flattened,
                  const std::string& x_label,
                  const std::string& y_label,
                  const std::string& z_label)
    {
        std::filesystem::path dir = _output_dir/std::to_string(_op_counter++);
        std::filesystem::create_directories(dir);
        binary_io::write_T(dir/"x_values.bin", x_values);
        binary_io::write_T(dir/"y_values.bin", y_values);
        binary_io::write_T(dir/"z_values.bin", z_values_flattened);

        nlohmann::json metadata;
        metadata["type"] = "graph_2d";
        metadata["x_label"] = x_label;
        metadata["y_label"] = y_label;
        metadata["z_label"] = z_label;
        json_io::write(dir/"metadata.json", metadata);
    }

    void run() const
    {
        run_cmd({ (std::filesystem::path(SOURCE_DIR)/".."/".venv"/"Scripts"/"python.exe").string(),
                  "\"" + (std::filesystem::path(TEST_DEPENDENCIES_DIR)/"vis"/"visualise.py").string() + "\"",
                  "--input", _output_dir.string() });
    }

private:
    void run_cmd(const std::vector<std::string>& args) const {
        std::stringstream ss;

        for (const std::string& arg : args) {
            ss << arg << " ";
        }

        std::cout << "ran command: " << ss.str() << std::endl;
        std::ignore = std::system(ss.str().c_str());
    }

private:
    size_t _op_counter = 0;
    std::filesystem::path _output_dir = TEST_RESOURCE_DIR;
};

// namespace visualise
// {
//     inline void run_cmd(const std::vector<std::string>& args) {
//         std::stringstream ss;
//
//         for (const std::string& arg : args) {
//             ss << arg << " ";
//         }
//
//         std::cout << "ran command: " << ss.str() << std::endl;
//         std::ignore = std::system(ss.str().c_str());
//     }
//
//     inline void graph_xy(const std::vector<float>& x_values, const std::vector<float>& y_values)
//     {
//         const auto output_dir = std::filesystem::path(TEST_RESOURCE_DIR);
//         io::bin::write_T(output_dir/"x_values.bin", x_values);
//         io::bin::write_T(output_dir/"y_values.bin", y_values);
//
//         run_cmd({ (std::filesystem::path(SOURCE_DIR)/".."/".venv"/"Scripts"/"python.exe").string(),
//                   "\"" + (std::filesystem::path(TEST_DEPENDENCIES_DIR)/"Visualise"/"visualise.py").string() + "\"",
//                   "--input", output_dir.string(),
//                   "--type graph_xy" });
//     }
// }
