#pragma once
#include "BlackScholes.h"
#include "Eigen"

namespace greeks
{
    inline Eigen::MatrixXf delta(const black_scholes::config& config,
                                 const black_scholes::solution& solution)
    {
        // Solver parameters.
        const float ds = config.stock_price_step();

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        const Eigen::MatrixXf& V = solution.option_price;
        Eigen::MatrixXf D(N, I);

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
        const float ds = config.stock_price_step();
        const float ds2 = ds * ds;

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        const Eigen::MatrixXf& V = solution.option_price;
        Eigen::MatrixXf G(N, I);

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

    inline Eigen::MatrixXf theta(const black_scholes::config& config,
                                 const black_scholes::solution& solution)
    {
        // Solver parameters.
        const float dt = config.time_step();

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        const Eigen::MatrixXf& V = solution.option_price;
        Eigen::MatrixXf Th(N, I);

        // Compute theta.
        for (size_t i = 0; i < I; ++i)
        {
            // S=0 boundary (forward difference).
            Th(0, i) = -(V(1, i) - V(0, i)) / dt;

            // Interior (central difference).
            for (size_t n = 1; n < N - 1; ++n)
                Th(n, i) = -(V(n + 1, i) - V(n - 1, i)) / (2.0f * dt);

            // S=S_max boundary (backward difference).
            Th(N - 1, i) = -(V(N - 1, i) - V(N - 2, i)) / dt;
        }

        return Th;
    }

    inline Eigen::MatrixXf vega(black_scholes::input input,
                                const black_scholes::config& config,
                                const black_scholes::solution& solution)
    {
        // Variables.
        const float& K = input.strike_price;
        const float  s = input.volatility;
        const float& r = input.interest_rate;

        // Solver parameters.
        const float& ds = 0.01f; // Volatility step.
        const float& S_max = config.stock_price_maximum;
        const float& S_min = config.stock_price_minimum;
        const float  dS    = config.stock_price_step();
        const float& T_start = config.time_start;
        const float& T_end   = config.time_end;
        const float  dT      = config.time_step();

        // Variables.
        const size_t N = solution.time.size();
        const size_t I = solution.stock_price.size();
        const auto V = [&](const size_t n, const size_t i, const float& h)
        {
            input.volatility = s + h;
            return black_scholes::closed_form_eq(input, config, T_start + n * dT, i * dS);
        };
        Eigen::MatrixXf Ve(N, I);

        // Initial condition:
        //  Ve(0, S) = 0 (as V(S, 0) does not depend on volatility).
        for (size_t i = 0; i < I; ++i)
            Ve(0, i) = 0.0f;

        // Boundary conditions:
        //  Ve(t, S_min) = S_min - K*exp(-r*tau)
        //  Ve(t, S_max) = S_max - K*exp(-r*tau)
        for (size_t n = 1; n < N; ++n)
        {
            const float tau = T_end - n * dT;
            Ve(n, 0) = black_scholes::closed_form_eq(input, config, S_min, tau);
            Ve(n, I - 1) = black_scholes::closed_form_eq(input, config, S_max, tau);
        }

        // Compute vega.
        for (size_t n = 1; n < N; ++n)
        {
            // Ve(n, 0)
        }

        return Ve;
    }
}
