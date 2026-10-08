#pragma once

namespace maths
{
    float standard_normal_CDF(const float& x)
    {
        return 0.5f * (1.0f + std::erf(x / std::sqrt(2.0f)));
    }

    constexpr float epsilon = 0.0001f;
}

namespace black_scholes
{
    struct input
    {
        float strike_price;
        float duration;
        float volatility;
        float interest_rate;
    };
    struct config
    {
        float maxmium_stock_price;
        float stock_price_step;
        float time_step;
    };
    struct solution
    {
        std::vector<float> option_price;
        std::vector<float> time;
        std::vector<float> stock_price;
    };

    inline float closed_form_eq(const black_scholes::input& input,
                                const float& current_stock_price,
                                const float& time)
    {
        // Variables.
        const auto N = maths::standard_normal_CDF;
        const float& S = current_stock_price;
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;
        const float tau = input.duration - time;

        // Since this formula contains a division by time (tau), this
        //  special case is needed.
        if (tau < maths::epsilon)
            return std::max(S - K, 0.0f);

        // General case.
        const float sqrt_tau = std::sqrt(tau);
        const float d0 = (std::log(S / K) + (r + s*s*0.5f) * tau) / (s * sqrt_tau);
        const float d1 = d0 - s * sqrt_tau;
        return N(d0) * S - N(d1) * K * std::exp(-r * tau);
    }

    inline black_scholes::solution closed_form_solver(const black_scholes::input& input,
                                                      const black_scholes::config& config)
    {
        // Variables.
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;
        const float& T = input.duration;

        // Solver parameters.
        const float& S_max = config.maxmium_stock_price;
        const float& ds = config.stock_price_step;
        const float& dt = config.time_step;

        // Calculate maximum index for time.
        size_t N = 0;
        for (float t = 0.0f; t < T; t += dt)
            ++N;

        // Calculate maximum index for stock price.
        size_t I = 0;
        for (float i = 0.0f; i < S_max; i += ds)
            ++I;

        // Calculate values.
        black_scholes::solution solution;
        for (size_t n = 0; n < N; ++n)
            for (size_t i = 0; i < I; ++i)
                solution.option_price.push_back(black_scholes::closed_form_eq(input, i * ds, T - n * dt));

        // Populate axes.
        //  NOTE: here we are inverting the transformation "tau = T - t".
        for (size_t n = 0; n < N; ++n) solution.time.push_back(n * dt);
        for (size_t i = 0; i < I; ++i) solution.stock_price.push_back(i * ds);

        return solution;
    }

    inline black_scholes::solution explicit_scheme_solver(const black_scholes::input& input,
                                                          const black_scholes::config& config)
    {
        // Variables.
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;
        const float& T = input.duration;

        // Solver parameters.
        const float& S_max = config.maxmium_stock_price;
        const float& ds = config.stock_price_step;
        const float& dt = config.time_step;

        // Calculate maximum index for time.
        size_t N = 0;
        for (float t = 0.0f; t < T; t += dt)
            ++N;

        // Calculate maximum index for stock price.
        size_t I = 0;
        for (float i = 0.0f; i < S_max; i += ds)
            ++I;

        // Value matrix.
        std::vector V(N, std::vector(I, 0.0f));

        // Initial condition:
        //  V(0, S) = max(S - K, 0)
        for (size_t i = 0; i < I; ++i)
            V[0][i] = std::max(i * ds - K, 0.0f);

        // Boundary conditions:
        //  V(t, 0) = 0
        //  V(t, S_max) = S_max - K*exp(-r*t)
        for (size_t n = 1; n < N; ++n)
        {
            V[n][0] = 0.0f;
            V[n][I - 1] = S_max - K * std::exp(-r * (T - n * dt));
        }

        // Calculate values.
        for (size_t n = 0; n < N - 1; ++n)
            for (size_t i = 1; i < I - 1; ++i)
                V[n + 1][i] = V[n][i] + dt *
                              (0.5f * s*s * (i * ds)*(i * ds) * ((
                               V[n][i + 1] - 2.0f * V[n][i] + V[n][i - 1]) / (ds * ds)) +
                               r * (i * ds) * ((V[n][i + 1] - V[n][i - 1]) / (2.0f * ds)) -
                               r * V[n][i]);

        // Populate axes.
        //  NOTE: for some reason here we don't need to invert the time transformation.
        black_scholes::solution solution;
        for (const auto& row : V)
            solution.option_price.insert(solution.option_price.end(), row.begin(), row.end());
        for (size_t n = 0; n < N; ++n) solution.time.push_back(n * dt);
        for (size_t i = 0; i < I; ++i) solution.stock_price.push_back(i * ds);

        return solution;
    }

    black_scholes::solution implicit_scheme_solver(const black_scholes::input& input,
                                                   const black_scholes::config& config);
}