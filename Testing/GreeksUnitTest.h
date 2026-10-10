#pragma once
#include <random>

#include "gtest/gtest.h"
#include "visualise.h"
#include "Greeks/BlackScholes.h"

class GreeksUnitTest : public ::testing::Test {};

TEST_F(GreeksUnitTest, TMP)
{
    black_scholes::input input;
    input.strike_price = 1.0f;
    input.duration = 1.0f;
    input.volatility = 0.5f;
    input.interest_rate = 0.01f;

    black_scholes::config config;
    config.maxmium_stock_price = 1.5f;
    config.time_step = 0.001f;
    config.stock_price_step = 0.05f;

    const black_scholes::solution solution =
        black_scholes::implicit_scheme_solver(input, config);
    const black_scholes::solution expected_solution =
        black_scholes::closed_form_solver(input, config);

    Eigen::VectorXf difference(solution.option_price.size());
    for (size_t i = 0; i < solution.option_price.size(); ++i)
        difference.data()[i] = solution.option_price.data()[i] - expected_solution.option_price.data()[i];

    visualise vis;
    vis.init();
    vis.graph_2d(solution.stock_price,          solution.time,          solution.option_price,
                 "stock_price",                 "time",                 "option_price");
    vis.graph_2d(expected_solution.stock_price, expected_solution.time, expected_solution.option_price,
                 "stock_price",                 "time",                 "option_price");
    vis.graph_2d(expected_solution.stock_price, expected_solution.time, difference,
                 "stock_price",                 "time",                 "option_price_diff");
    vis.run();
}