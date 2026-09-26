#pragma once
#include <random>

template <typename DistributionT = std::uniform_real_distribution<float>>
auto random(const typename DistributionT::result_type& min, const typename DistributionT::result_type& max)
{
    thread_local std::mt19937 rng{ std::random_device{}() };
    DistributionT dist{ min, max };

    return dist(rng);
}