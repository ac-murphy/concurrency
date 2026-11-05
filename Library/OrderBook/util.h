#pragma once
#include <chrono>
#include <random>

// template <typename DistributionT = std::uniform_real_distribution<float>>
// auto random(const typename DistributionT::result_type& min, const typename DistributionT::result_type& max)
// {
//     thread_local std::mt19937 rng{ std::random_device{}() };
//     DistributionT dist{ min, max };
//
//     return dist(rng);
// }

#define RECORD(x) \
    { \
        auto start = std::chrono::high_resolution_clock::now(); \
        { x; } \
        auto end = std::chrono::high_resolution_clock::now(); \
        std::cout << std::format("[{}μs], {}", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(), #x) << std::endl; \
    }
