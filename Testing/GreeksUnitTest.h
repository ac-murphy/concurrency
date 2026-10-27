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
        /* maximum stock price */ 1.5f,
        /* stock_price_step */    0.05f,
        /* time_step */           0.001f
    };
    inline static black_scholes::input input = {
        /* strike price */  1.0f,
        /* duration */      1.0f,
        /* volatility */    0.5f,
        /* interest rate */ 0.01f
    };
    inline static black_scholes::solution closed_form_solution = black_scholes::closed_form_solver(input, config);
    inline static black_scholes::solution implicit_solution    = black_scholes::explicit_scheme_solver(input, config);
    inline static black_scholes::solution explicit_solution    = black_scholes::implicit_scheme_solver(input, config);
};

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

    // Solutions for V (option price).
    const black_scholes::solution solution = black_scholes::implicit_scheme_solver(input, config);
    const black_scholes::solution expected_solution = black_scholes::closed_form_solver(input, config);
    const Eigen::MatrixXf difference = solution.option_price - expected_solution.option_price;

    // Derived values (Greeks).
    const Eigen::MatrixXf delta = greeks::delta(config, solution);
    const Eigen::MatrixXf gamma = greeks::gamma(config, solution);

    vis::visualise_plotting vis;

    vis.begin_frame("implicit scheme solver");
    {
        vis.set_axis_labels("stock_price", "time", "option_price");
        // vis::plot_3D<float> plot{solution.stock_price, solution.time, solution.option_price};
        vis::plot_3D<float> plot;
        plot.x_values = solution.stock_price;
        plot.y_values = solution.time;
        plot.z_values = solution.option_price;
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add_plot(plot);
    }
    vis.end_frame();

    vis.begin_frame("$\\Delta$, $\\Gamma$ and $V$ against $S$");
    {
        vis.set_axis_labels("stock price (£)", "option price (£)");
        vis.set_legend_visible(true);

        const auto& stock_price = solution.stock_price;
        const auto& option_price = solution.option_price;
        const auto& time = solution.time;

        vis::plot_2D<float> plot_V0;
        plot_V0.label = "$V(t=0, S)$";
        plot_V0.x_values = stock_price;
        plot_V0.y_values = option_price.row(0);

        vis::plot_2D<float> plot_V0_5;
        plot_V0_5.label = "$V(t=0.5, S)$";
        plot_V0_5.x_values = stock_price;
        plot_V0_5.y_values = option_price.row(0);

        vis::plot_2D<float> plot_V1;
        plot_V1.label = "$V(t=1, S)$";
        plot_V1.x_values = stock_price;
        plot_V1.y_values = option_price.row(0);

        vis.add_plot(plot_V0);
        vis.add_plot(plot_V0_5);
        vis.add_plot(plot_V1);
    }
    vis.end_frame();

    /* vis.begin_frame("closed form solver");
    {
        vis.set_axis_labels("stock_price", "time", "option_price");
        vis::plot_3D<float> plot(expected_solution.stock_price, expected_solution.time, expected_solution.option_price);
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add_plot(plot);
    }
    vis.end_frame();

    vis.begin_frame("solver difference");
    {
        vis.set_axis_labels("stock_price", "time", "option_price_difference");
        vis::plot_3D<float> plot(expected_solution.stock_price, expected_solution.time, difference);
        plot.data_storage_order = vis::storage_order::column_major;
        vis.add_plot(plot);
    }
    vis.end_frame(); */

    vis.run();
}
TEST_F(GreeksUnitTest, OptionPrice_StockPrice_ClosedForm)
{
    vis::visualise_plotting vis;
    vis.begin_frame("option price surface");
    {
        vis.set_axis_labels("time", "stock price", "option price");
        vis.set_legend_visible(true);

        vis::plot_3D<float> plot;
        plot.x_values = closed_form_solution.time;
        plot.y_values = closed_form_solution.stock_price;
        plot.z_values = closed_form_solution.option_price;
        plot.label = "$V(t, S)$";

        vis.add_plot(plot);
    }
    vis.end_frame();
    vis.begin_frame("option price vs stock price");
    {
        vis.set_axis_labels("stock price", "option price");
        vis.set_legend_visible(true);

        vis::plot_2D<float> plot_t0;
        plot_t0.label    = "$V(t=0, S)$";
        plot_t0.x_values = closed_form_solution.stock_price;
        plot_t0.y_values = closed_form_solution.option_price.row(0);

        vis::plot_2D<float> plot_t500;
        plot_t500.label  = "$V(t=0.5, S)$";
        plot_t500.x_values = closed_form_solution.stock_price;
        plot_t500.y_values = closed_form_solution.option_price.row(500);

        vis::plot_2D<float> plot_t1000;
        plot_t1000.label = "$V(t=1, S)$";
        plot_t1000.x_values = closed_form_solution.stock_price;
        plot_t1000.y_values = closed_form_solution.option_price.row(1000);

        vis.add_plot(plot_t0);
        vis.add_plot(plot_t500);
        vis.add_plot(plot_t1000);
    }
    vis.end_frame();
    vis.run();
}