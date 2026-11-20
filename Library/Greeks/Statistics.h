#pragma once

#include "Eigen"

namespace stats
{
    inline Eigen::VectorXf volatility(const std::vector<price_type>& stock_prices, const size_t& kernel_size = 20)
    {
        const auto& W = kernel_size;
        const auto& S = stock_prices;
        const size_t N = stock_prices.size();

        Eigen::VectorXf v(N);
        v.fill(0.0f);
        if (N <= kernel_size)
            return v;

        // Calculate log-returns for each stock price pair.
        Eigen::VectorXf log_returns(N);
        log_returns.fill(0.0f);
        for (size_t i = 0; i < N - 1; ++i)
        {
            const auto& S0 = static_cast<float>(S[i]);
            const auto& S1 = static_cast<float>(S[i + 1]);
            log_returns[i] = std::log(S1 / S0);
        }

        // Calculate volatility for each kernel.
        for (size_t i = 0; i < N - W; ++i)
        {
            // Calculate mean log-returns in kernel.
            float mean_log_returns = 0.0f;
            for (size_t j = 0; j < W; ++j)
                mean_log_returns += log_returns[i + j];

            // Calculate squared sum of each "log-return - mean-log-return" in kernel with mean.
            mean_log_returns /= static_cast<float>(W);
            for (size_t j = 0; j < W; ++j)
            {
                const float mean_diff = log_returns[i + j] - mean_log_returns;
                v[i] += mean_diff * mean_diff;
            }

            // Calculate volatility.
            v[i] *= 252.0f / (static_cast<float>(W) - 1.0f);
            v[i] = std::sqrt(v[i]);
        }

        return v;
    }
}