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
        float volatility;
        float interest_rate;
    };
    struct variable_input
    {
        float strike_price;
        Eigen::VectorXf volatility;
        Eigen::VectorXf interest_rate;
    };
    struct config
    {
        float stock_price_minimum;
        float stock_price_maximum;
        size_t stock_price_steps;
        float time_start;
        float time_end;
        size_t time_steps;

        float stock_price_step() const
        {
            return (stock_price_maximum - stock_price_minimum) / (stock_price_steps - 1);
        }

        float time_step() const
        {
            return (time_end - time_start) / (time_steps - 1);
        }
    };
    struct solution
    {
        Eigen::MatrixXf option_price;
        Eigen::VectorXf time;
        Eigen::VectorXf stock_price;

        solution(const size_t& N, const size_t& I)
        : option_price(N, I), time(N), stock_price(I)
        {}

        float operator()(const float& time_, const float& stock_price_)
        {
            // Find lower bound index for time.
            size_t t = 0;
            if (time_ <= time.minCoeff())
                t = 0;
            else if (time_ >= time.maxCoeff())
                t = time.size() - 1;
            else
                while (time[t + 1] < time_)
                    t++;

            // Find lower bound index for stock price.
            size_t i = 0;
            if (stock_price_ <= stock_price.minCoeff())
                i = 0;
            else if (stock_price_ >= stock_price.maxCoeff())
                i = stock_price.size() - 1;
            else
                while (stock_price[i + 1] < stock_price_)
                    i++;

            // Calculate weights in both axes.
            const float w_stock_price = (stock_price_ - stock_price[i]) / (stock_price[i + 1] - stock_price[i]);
            const float w_time = (time_ - time[t]) / (time[t + 1] - time[t]);

            // Bilinearly interpolate values.
            return   (1.0f - w_stock_price) * (1.0f - w_time) * option_price(t, i)
                   + w_stock_price          * (1.0f - w_time) * option_price(t, i + 1)
                   + (1.0f - w_stock_price) * w_time          * option_price(t + 1, i)
                   + w_stock_price          * w_time          * option_price(t + 1, i + 1);
        }
    };

    inline float closed_form_eq(const black_scholes::input& input,
                                const black_scholes::config& config,
                                const float& current_stock_price,
                                const float& time)
    {
        // Variables.
        const auto N = maths::standard_normal_CDF;
        const float& S = current_stock_price;
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;
        const float duration = config.time_end - config.time_start;
        const float tau = config.time_start + duration - time;

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
        // const float& T = input.duration;

        // Solver parameters.
        const float& S_max = config.stock_price_maximum;
        const float& S_min = config.stock_price_minimum;
        const float ds     = config.stock_price_step();
        const size_t& I    = config.stock_price_steps;
        const float& T_start = config.time_start;
        const float& T_end   = config.time_end;
        const float dt       = config.time_step();
        const size_t& N      = config.time_steps;



        // Calculate values.
        black_scholes::solution solution(N, I);
        for (size_t n = 0; n < N; ++n)
            for (size_t i = 0; i < I; ++i)
                solution.option_price(n, i) = black_scholes::closed_form_eq(input, config, S_min + i * ds, T_end - n * dt);

        // Populate axes.
        for (size_t n = 0; n < N; ++n) solution.time(n) = T_start + n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = S_min + i * ds;

        return solution;
    }

    inline black_scholes::solution explicit_scheme_solver(const black_scholes::input& input,
                                                          const black_scholes::config& config)
    {
        // Variables.
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;

        // Solver parameters.
        const float& S_max = config.stock_price_maximum;
        const float& S_min = config.stock_price_minimum;
        const float ds     = config.stock_price_step();
        const size_t& I    = config.stock_price_steps;
        const float& T_start = config.time_start;
        const float& T_end   = config.time_end;
        const float dt       = config.time_step();
        const size_t& N      = config.time_steps;

        // Value matrix.
        black_scholes::solution solution(N, I);
        auto& V = solution.option_price;

        // Initial condition:
        //  V(0, S) = max(S - K, 0)
        for (size_t i = 0; i < I; ++i)
            V(0, i) = std::max(S_min + i * ds - K, 0.0f);

        // Boundary conditions:
        //  V(t, 0) = 0
        //  V(t, S_max) = S_max - K*exp(-r*t) - note: instead we are using the exact solution at this boundary.
        for (size_t n = 1; n < N; ++n)
        {
            const float tau = T_end - n * dt;
            V(n, 0) = black_scholes::closed_form_eq(input, config, S_min, tau);
            V(n, I - 1) = black_scholes::closed_form_eq(input, config, S_max, tau);
        }

        // Calculate values.
        for (size_t n = 0; n < N - 1; ++n)
            for (size_t i = 1; i < I - 1; ++i)
                V(n + 1, i) = V(n, i) + dt *
                              (0.5f * s*s * (S_min + i * ds)*(S_min + i * ds) * ((
                               V(n, i + 1) - 2.0f * V(n, i) + V(n, i - 1)) / (ds * ds)) +
                               r * (S_min + i * ds) * ((V(n, i + 1) - V(n, i - 1)) / (2.0f * ds)) -
                               r * V(n, i));

        // Populate axes.
        for (size_t n = 0; n < N; ++n) solution.time(n) = T_start + n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = S_min + i * ds;

        return solution;
    }

    inline black_scholes::solution implicit_scheme_solver(const black_scholes::input& input,
                                                          const black_scholes::config& config)
    {
        // Variables.
        const float& K = input.strike_price;
        const float& s = input.volatility;
        const float& r = input.interest_rate;

        // Solver parameters.
        const float& S_max = config.stock_price_maximum;
        const float& S_min = config.stock_price_minimum;
        const float  ds    = config.stock_price_step();
        const float& T_start = config.time_start;
        const float& T_end   = config.time_end;
        const float  dt      = config.time_step();

        // Calculate maximum index for time.
        const size_t& N = config.time_steps; // 0;
        // for (float t = T_start; t < T_end; t += dt)
        //     ++N;

        // Calculate maximum index for stock price.
        const size_t& I = config.stock_price_steps; // 0;
        // for (float i = S_min; i < S_max; i += ds)
        //     ++I;

        // Value matrix.
        black_scholes::solution solution(N, I);
        auto& V = solution.option_price;

        // todo: possibly redundant.
        V.setZero();

        // Coefficients.
        auto alpha = [&](const size_t& n, const size_t& i)
        {
            return -((i * dt) / (2.0f)) * (((i * s*s)) - r);
        };
        auto beta = [&](const size_t& n, const size_t& i)
        {
            return 1.0f + dt * ((s*s * i * i) + r);
        };
        auto gamma = [&](const size_t& n, const size_t& i)
        {
            return -((i * dt) / (2.0f)) * (((i * s*s)) + r);
        };

        // Initial condition.
        //  V(0, S) = max(S - K, 0)
        for (size_t i = 0; i < I; ++i)
            V(0, i) = std::max(S_min + i * ds - K, 0.0f);

        // Solve system "Ax = b" for each 0 < n <= N.
        for (size_t n = 1; n < N; ++n)
        {
            // Values.
            const float lower_boundary = black_scholes::closed_form_eq(input, config, S_min, T_end - n * dt);
            const float upper_boundary = black_scholes::closed_form_eq(input, config, S_max, T_end - n * dt);
            const size_t _I = I - 2;

            // Populate "b".
            Eigen::VectorXf b = V.row(n - 1).segment(1, _I).transpose();
            b[0]     -= alpha(n, 0) * lower_boundary;
            b[_I - 1] -= gamma(n, _I - 1) * upper_boundary;

            // Populate "A".
            Eigen::SparseMatrix<float> A(_I, _I);
            for (size_t i = 0; i < _I; ++i)
            {
                if (i > 0)      A.coeffRef(i, i - 1)  = alpha(n, i);
                                A.coeffRef(i, i)      = beta(n, i);
                if (i + 1 < _I) A.coeffRef(i, i + 1)  = gamma(n, i);
            }

            // Solve system for "x".
            Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
            solver.compute(A);

            auto x = solver.solve(b);

            V.row(n)[0] = lower_boundary;
            V.row(n).segment(1, _I) = x.transpose();
            V.row(n)[I - 1] = upper_boundary;
        }

        for (size_t n = 0; n < N; ++n) solution.time(n) = T_start + n * dt;
        for (size_t i = 0; i < I; ++i) solution.stock_price(i) = S_min + i * ds;

        return solution;
    }

    inline black_scholes::solution crank_nicholson_solver(const black_scholes::variable_input& input,
                                                          const black_scholes::config& config)
    {
        // Solver parameters.
        const float& S_max = config.stock_price_maximum;
        const float& S_min = config.stock_price_minimum;
        const float dS     = config.stock_price_step();
        const float& T_start = config.time_start;
        const float& T_end   = config.time_end;
        const float dT       = config.time_step();

        // Variables.
        const float& K = input.strike_price;
        const auto& s = input.volatility;
        const auto& r = input.interest_rate;
        const size_t& N = config.time_steps;
        const size_t& I = config.stock_price_steps;
        const float duration = T_end - T_start;

        // Solution variable.
        black_scholes::solution solution(N, I);
        auto& V = solution.option_price;

        // Generate time values.
        auto& T = solution.time;
        for (size_t idx = 0; idx < N; ++idx)
            T(idx) = T_start + idx * dT;

        // Generate stock price values.
        auto& S = solution.stock_price;
        for (size_t idx = 0; idx < I; ++idx)
            S(idx) = S_min + idx * dS;

        auto alpha = [&](const size_t& n, const size_t& i)
        {
            return s(n) * s(n) * S(i) * S(i) / (2.0f * dS * dS) - (r(n) * S(i)) / (2.0f * dS);
        };
        auto beta = [&](const size_t& n, const size_t& i)
        {
            return -(s(n) * s(n) * S(i) * S(i)) / (dS * dS) - r(n);
        };
        auto gamma = [&](const size_t& n, const size_t& i)
        {
            return s(n) * s(n) * S(i) * S(i) / (2.0f * dS * dS) + (r(n) * S(i)) / (2.0f * dS);
        };

        const auto I_interior = I - 2;
        Eigen::MatrixXf unknown_coeff_M = Eigen::MatrixXf::Zero(I_interior, I_interior);
        Eigen::MatrixXf known_coeff_M = Eigen::MatrixXf::Zero(I_interior, I_interior);

        // Initial condition.
        //  V(0, S) = max(S - K, 0).
        for (size_t i = 0; i < I; ++i)
        {
            V(0, i) = std::max(S(i) - K, 0.0f);
        }

        // Solve interior.
        for (size_t n = 1; n < N; ++n)
        {
            // Variables.
            const float tau = duration - T(n);

            // Boundary conditions.
            V(n, 0)     = 0.0f;
            V(n, I - 1) = S_max - K * std::exp(-r(n) * tau);;

            // Build matrices.
            for (size_t i = 0; i < I_interior; ++i)
            {
                if (i > 0)              unknown_coeff_M(i, i - 1) =      - 0.5f * dT * alpha(n,     i + 1);
                                        unknown_coeff_M(i, i    ) = 1.0f - 0.5f * dT *  beta(n,     i + 1);
                if (i + 1 < I_interior) unknown_coeff_M(i, i + 1) =      - 0.5f * dT * gamma(n,     i + 1);

                if (i > 0)              known_coeff_M(i, i - 1) =        0.5f * dT * alpha(n - 1, i + 1);
                                        known_coeff_M(i, i    ) = 1.0f + 0.5f * dT *  beta(n - 1, i + 1);
                if (i + 1 < I_interior) known_coeff_M(i, i + 1) =        0.5f * dT * gamma(n - 1, i + 1);
            }

            Eigen::VectorXf rhs = known_coeff_M * V.row(n - 1).segment(1, I_interior).transpose();
            rhs(0)              += 0.5f * dT * alpha(n, 1    ) * (V(n, 0    ) + V(n - 1, 0    ));
            rhs(I_interior - 1) += 0.5f * dT * gamma(n, I - 2) * (V(n, I - 1) + V(n - 1, I - 1));

            Eigen::VectorXf x = unknown_coeff_M.partialPivLu().solve(rhs);
            V.row(n).segment(1, I_interior) = x.transpose();
        }

        return solution;
    }

    inline black_scholes::solution crank_nicholson_solver(const black_scholes::input& input,
                                                          const black_scholes::config& config)
    {
        const size_t N = config.time_steps;
        const size_t I = config.stock_price_steps;

        black_scholes::variable_input variable_input = {
            .strike_price = input.strike_price,
            .volatility = Eigen::VectorXf::Constant(N, input.volatility),
            .interest_rate = Eigen::VectorXf::Constant(N, input.interest_rate),
        };

        return black_scholes::crank_nicholson_solver(variable_input, config);
    }
}