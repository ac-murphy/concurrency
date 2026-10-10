#pragma once
#include "BlackScholes.h"
#include "Eigen"

namespace greeks
{
    inline Eigen::MatrixXf delta(const black_scholes::config& config,
                                 const black_scholes::solution& solution)
    {
        // Solver parameters.
        const float& ds = config.stock_price_step;

        // Output.
        std::vector<float> delta_values;

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        Eigen::MatrixXf D(N, I);
        Eigen::Map<const Eigen::MatrixXf> V(solution.option_price.data(), N, I);

        // Compute delta.
        for (size_t n = 0; n < N; ++n)
        {
            // S=0 boundary (forward difference).
            D(n, 0) = (V(n, 1) - V(n, 0)) / ds;

            // Interior (central difference).
            for (size_t i = 1; i < I - 1; ++i)
                D(n, i) = (V(n, i + 1) - V(n, i - 1)) / (2.0f * ds);

            // S=S_max boundary (backward difference).
            D(n, I - 1) = (V(n, I - 1) - V(n, I - 2)) / ds;
        }

        return D;
    }

    inline Eigen::MatrixXf gamma(const black_scholes::config& config,
                                 const black_scholes::solution& solution)
    {
        // Solver parameters.
        const float& ds = config.stock_price_step;
        const float ds2 = ds * ds;

        // Output.
        std::vector<float> delta_values;

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        Eigen::MatrixXf G(N, I);
        Eigen::Map<const Eigen::MatrixXf> V(solution.option_price.data(), N, I);

        // Compute gamma.
        for (size_t n = 0; n < N; ++n)
        {
            // S=0 boundary (forward difference).
            G(n, 0) = (2.0f*V(n, 0) - 5.0f*V(n, 1) + 4.0f*V(n, 2) - V(n, 3)) / ds2;

            // Interior (central difference).
            for (size_t i = 1; i < I - 1; ++i)
                G(n, i) = (V(n, i + 1) - 2.0f*V(n, i) + V(n, i - 1)) / ds2;

            // S=S_max boundary (backward difference).
            G(n, I - 1) = (2.0f*V(n, I - 1) - 5.0f*V(n, I - 2) + 4.0f*V(n, I - 3) - V(n, I - 4)) / ds2;
        }

        return G;

    }
}
