#pragma once
#include "Eigen"

namespace maths
{
    inline float standard_normal_CDF(const float& x)
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
        Eigen::MatrixXf option_price;
        Eigen::VectorXf time;
        Eigen::VectorXf stock_price;

        solution(const size_t& N, const size_t& I)
        : option_price(N, I), time(N), stock_price(I)
        {}
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
        black_scholes::solution solution(N, I);
        for (size_t n = 0; n < N; ++n)
            for (size_t i = 0; i < I; ++i)
                solution.option_price(n, i) = black_scholes::closed_form_eq(input, i * ds, T - n * dt);

        // Populate axes.
        for (size_t n = 0; n < N; ++n) solution.time(n) = n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = i * ds;

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
        // std::vector V(N, std::vector(I, 0.0f));
        black_scholes::solution solution(N, I);
        auto& V = solution.option_price;

        // Initial condition:
        //  V(0, S) = max(S - K, 0)
        for (size_t i = 0; i < I; ++i)
            V(0, i) = std::max(i * ds - K, 0.0f);

        // Boundary conditions:
        //  V(t, 0) = 0
        //  V(t, S_max) = S_max - K*exp(-r*t) - note: instead we are using the exact solution at this boundary.
        for (size_t n = 1; n < N; ++n)
        {
            V(n, 0) = 0.0f;
            V(n, I - 1) = black_scholes::closed_form_eq(input, S_max, T - n * dt);
        }

        // Calculate values.
        for (size_t n = 0; n < N - 1; ++n)
            for (size_t i = 1; i < I - 1; ++i)
                V(n + 1, i) = V(n, i) + dt *
                              (0.5f * s*s * (i * ds)*(i * ds) * ((
                               V(n, i + 1) - 2.0f * V(n, i) + V(n, i - 1)) / (ds * ds)) +
                               r * (i * ds) * ((V(n, i + 1) - V(n, i - 1)) / (2.0f * ds)) -
                               r * V(n, i));

        // Populate axes.
        //  todo: could substitute "i * ds" for value in solution.stock_price in above calculation.
        for (size_t n = 0; n < N; ++n) solution.time(n) = n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = i * ds;

        return solution;
    }

    inline black_scholes::solution implicit_scheme_solver(const black_scholes::input& input,
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
        black_scholes::solution solution(N, I);
        auto& V = solution.option_price;

        // todo: possibly redundant.
        V.setZero();

        // Coefficients.
        auto alpha = [&](const size_t& i)
        {
            return -((i * dt) / (2.0f)) * (((i * s*s)) - r);
        };
        auto beta = [&](const size_t& i)
        {
            return 1.0f + dt * ((s*s * i * i) + r);
        };
        auto gamma = [&](const size_t& i)
        {
            return -((i * dt) / (2.0f)) * (((i * s*s)) + r);
        };

        // Initial condition.
        //  V(0, S) = max(S - K, 0)
        for (size_t i = 0; i < I; ++i)
            V(0, i) = std::max(i * ds - K, 0.0f);

        // Solve system "Ax = b" for each 0 <= n <= N.
        for (size_t n = 1; n < N; ++n)
        {
            // Values.
            const float lower_boundary = 0.0f;
            const float upper_boundary = black_scholes::closed_form_eq(input, S_max, T - n * dt);
            const size_t _I = I - 2;

            // Populate "b".
            Eigen::VectorXf b = V.row(n - 1).segment(1, _I).transpose();
            b[0]     -= alpha(0) * lower_boundary;
            b[_I - 1] -= gamma(_I - 1) * upper_boundary;

            // Populate "A".
            Eigen::SparseMatrix<float> A(_I, _I);
            for (size_t i = 0; i < _I; ++i)
            {
                if (i > 0)     A.coeffRef(i, i - 1) = alpha(i);
                A.coeffRef(i, i)     = beta(i);
                if (i + 1 < _I) A.coeffRef(i, i + 1) = gamma(i);
            }

            // Solve system for "x".
            Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
            solver.compute(A);

            auto x = solver.solve(b);

            V.row(n)[0] = lower_boundary;
            V.row(n).segment(1, _I) = x.transpose();
            V.row(n)[I - 1] = upper_boundary;
        }

        // Populate axes.
        // for (size_t n = 0; n < N; ++n)
        //     for (size_t i = 0; i < I; ++i)
        //         solution.option_price.push_back(V(n, i));

        for (size_t n = 0; n < N; ++n) solution.time(n) = n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = i * ds;

        return solution;
    }
}