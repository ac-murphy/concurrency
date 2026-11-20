#pragma once
#include <random>

#include "gtest/gtest.h"
#include "Greeks/BlackScholes.h"
#include "Greeks/Greeks.h"
#include "Visualise.h"

class GreeksUnitTest : public ::testing::Test
{
public:
    inline static black_scholes::config config = {
        .stock_price_minimum = 0.0f,
        .stock_price_maximum = 1.5f,
        .stock_price_steps   = 30,
        .time_start          = 0.0f,
        .time_end            = 1.0f,
        .time_steps          = 1000
    };
    inline static black_scholes::input input = {
        .strike_price  = 1.0f,
        .volatility    = 0.5f,
        .interest_rate = 0.01f
    };
    inline static black_scholes::solution CF_solution = black_scholes::closed_form_solver(input, config);
    inline static black_scholes::solution ES_solution = black_scholes::implicit_scheme_solver(input, config);
    inline static black_scholes::solution IS_solution = black_scholes::explicit_scheme_solver(input, config);
    inline static black_scholes::solution CN_solution = black_scholes::crank_nicholson_solver(input, config);

public:
    using black_scholes_solver = std::function<black_scholes::solution(black_scholes::input, black_scholes::config)>;

    static void converges(black_scholes_solver solver, const size_t& time_steps, const size_t& stock_price_steps)
    {
        auto input = GreeksUnitTest::input;
        auto config = GreeksUnitTest::config;
        for (float factor = 1.0f; factor < 2.0f; ++factor)
        {
            config.time_steps = time_steps * factor;
            config.stock_price_steps = stock_price_steps * factor;
            black_scholes::solution solution = solver(input, config);

            Eigen::MatrixXf diff(config.time_steps, config.stock_price_steps);
            for (size_t n = 0; n < config.time_steps; ++n)
                for (size_t i = 0; i < config.stock_price_steps; ++i)
                    diff(n, i) = solution(n, i) - black_scholes::closed_form_eq(input, config, solution.stock_price(i), solution.time(n));

            std::cout << diff.sum() << std::endl;
        }
    }
};

TEST_F(GreeksUnitTest, TMP2)
{
    black_scholes::input input = {
        .strike_price  = 1.0f,
        .volatility    = 0.5f,
        .interest_rate = 0.01f
    };
    black_scholes::config config = {
        .stock_price_minimum = 0.5f,
        .stock_price_maximum = 2.0f,
        .stock_price_steps   = 30,
        .time_start          = 0.5f,
        .time_end            = 1.0f,
        .time_steps          = 1000
    };

    // Solutions for V (option price).
    const black_scholes::solution solution = black_scholes::crank_nicholson_solver(input, config);
    const black_scholes::solution expected_solution = black_scholes::explicit_scheme_solver(input, config);
    const Eigen::MatrixXf difference = solution.option_price - expected_solution.option_price;

    vis::visualise_plotting vis;

    vis.begin_frame("option price (CF); $V(S, \\tau)$");
    {
        vis.set_axis_labels("stock_price", "time", "option_price");
        vis::plot_3D<float> plot;
        plot.x_values = expected_solution.stock_price;
        plot.y_values = expected_solution.time;
        plot.z_values = expected_solution.option_price;
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("option price (IS); $V(S, \\tau)$");
    {
        vis.set_axis_labels("stock_price", "time", "option_price");
        vis::plot_3D<float> plot;
        plot.x_values = solution.stock_price;
        plot.y_values = solution.time;
        plot.z_values = solution.option_price;
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("option price (diff); $V(S, \\tau)$");
    {
        vis.set_axis_labels("stock_price", "time", "option_price");
        vis::plot_3D<float> plot;
        plot.x_values = expected_solution.stock_price;
        plot.y_values = expected_solution.time;
        plot.z_values = difference;
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("rate of change of option price ($V$) w.r.t stock price ($S$); $\\Delta(S, \\tau)$");
    {
        vis.set_axis_labels("stock price (£)", "time to maturity ($\\tau$)", "rate of change in $V$ (dimensionless)");
        vis::plot_3D<float> plot;
        plot.x_values = solution.stock_price;
        plot.y_values = solution.time;
        plot.z_values = greeks::delta(config, solution);
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("rate of change of $\\Delta$ w.r.t. stock price ($S$); $\\Gamma(S, \\tau)$");
    {
        vis.set_axis_labels("stock price (£)", "time to maturity ($\\tau$)", "rate of change in $\\Delta$ (£$^{-1}$)");
        vis::plot_3D<float> plot;
        plot.x_values = solution.stock_price;
        plot.y_values = solution.time;
        plot.z_values = greeks::gamma(config, solution);
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("rate of change of option price ($V$) w.r.t time ($\\tau$); $\\Theta(S, \\tau)$");
    {
        vis.set_axis_labels("stock price (£)", "time to maturity ($\\tau$)", "rate of change in $V$ ($£/t$)");
        vis::plot_3D<float> plot;
        plot.x_values = solution.stock_price;
        plot.y_values = solution.time;
        plot.z_values = greeks::theta(config, solution);
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add(plot);
    }
    vis.end_frame();

    // vis.run();
}

TEST_F(GreeksUnitTest, ExplicitScheme_Correctness)
{
    Eigen::MatrixXf option_price_difference = CF_solution.option_price - ES_solution.option_price;

    vis::visualise_plotting vis;

    vis.begin_frame("CF solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = CF_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("ES solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = ES_solution.stock_price;
        plot.y_values = ES_solution.time;
        plot.z_values = ES_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("solution diff.");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = option_price_difference;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.run();
}
TEST_F(GreeksUnitTest, ImplicitScheme_Correctness)
{
    Eigen::MatrixXf option_price_difference = CF_solution.option_price - IS_solution.option_price;

    vis::visualise_plotting vis;

    vis.begin_frame("CF solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = CF_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("IS solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = IS_solution.stock_price;
        plot.y_values = IS_solution.time;
        plot.z_values = IS_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("solution diff.");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = option_price_difference;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.run();
}
TEST_F(GreeksUnitTest, CrankNicholson_Correctness)
{
    Eigen::MatrixXf option_price_difference = CF_solution.option_price - CN_solution.option_price;

    vis::visualise_plotting vis;

    vis.begin_frame("CF solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = CF_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("CN solution");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CN_solution.stock_price;
        plot.y_values = CN_solution.time;
        plot.z_values = CN_solution.option_price;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.begin_frame("solution diff.");
    {
        vis::plot_3D<float> plot;
        plot.x_values = CF_solution.stock_price;
        plot.y_values = CF_solution.time;
        plot.z_values = option_price_difference;
        plot.data_storage_order = vis::column_major;

        vis.set_axis_labels("stock_price", "time_to_maturity", "option_price");
        vis.add(plot);
    }
    vis.end_frame();

    vis.run();
}


TEST_F(GreeksUnitTest, TEMP2)
{
    std::cout << CN_solution.option_price.col(CN_solution.stock_price.size() - 1) << std::endl;
}