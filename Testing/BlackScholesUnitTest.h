#pragma once
#include "GreeksUnitTest.h"
#include "Greeks/BlackScholes.h"
#include "gtest/gtest.h"
#include "Visualise.h"

class BlackScholesUnitTest : public ::testing::Test
{
private:
    using black_scholes_solver = std::function<black_scholes::solution(black_scholes::input, black_scholes::config)>;

public:
    struct converges_parameters
    {
        black_scholes_solver solver;
        std::string solver_name;
        size_t time_steps;
        size_t stock_price_steps;
        size_t iterations;
    };

    static void converges(const converges_parameters& parameters)
    {
        auto&& [solver, solver_name, time_steps, stock_price_steps, iterations] = parameters;
        auto input = GreeksUnitTest::input;
        std::vector<float> mean_diff_time, mean_diff_stock_price;
        std::vector<size_t> indices;
        for (size_t idx = 0; idx < iterations; ++idx)
        {
            const float factor = idx + 1.0f;

            // Convergence as stock price step shrinks.
            black_scholes::config config_stock_price = GreeksUnitTest::config;
            config_stock_price.stock_price_steps = stock_price_steps * factor;
            black_scholes::solution solution_stock_price = solver(input, config_stock_price);
            Eigen::MatrixXf diff_stock_price(config_stock_price.time_steps, config_stock_price.stock_price_steps);
            for (size_t n = 0; n < config_stock_price.time_steps; ++n)
                for (size_t i = 0; i < config_stock_price.stock_price_steps; ++i)
                    diff_stock_price(n, i) = solution_stock_price.option_price(n, i) - black_scholes::closed_form_eq(input, config_stock_price, solution_stock_price.stock_price(i), solution_stock_price.time(n));

            // Convergence as time step shrinks.
            black_scholes::config config_time = GreeksUnitTest::config;
            config_time.time_steps = time_steps * factor;
            black_scholes::solution solution_time = solver(input, config_time);
            Eigen::MatrixXf diff_time(config_time.time_steps, config_time.stock_price_steps);
            for (size_t n = 0; n < config_time.time_steps; ++n)
                for (size_t i = 0; i < config_time.stock_price_steps; ++i)
                    diff_time(n, i) = solution_time.option_price(n, i) - black_scholes::closed_form_eq(input, config_time, solution_time.stock_price(i), solution_time.time(n));

            // Record mean values.
            mean_diff_stock_price.push_back(std::abs(diff_stock_price.sum() / diff_stock_price.size()));
            mean_diff_time.push_back(std::abs(diff_time.sum() / diff_time.size()));
            indices.push_back(idx);
        }

        vis::visualise_plotting vis;

        vis.begin_frame(std::format("convergence of \"{}\" (in time)", solver_name));
        {
            vis::plot_2D<size_t, float> plot;
            plot.x_values = indices;
            plot.y_values = mean_diff_time;
            plot.label = "abs. mean difference (time $t$)";

            vis.set_legend_visible(true);
            vis.set_axis_scale_types(vis::linear, vis::logarithmic);
            vis.set_axis_labels("iteration", "difference");
            vis.add(plot);
        }
        vis.end_frame();

        vis.begin_frame(std::format("convergence of \"{}\" (in space)", solver_name));
        {
            vis::plot_2D<size_t, float> plot;
            plot.x_values = indices;
            plot.y_values = mean_diff_stock_price;
            plot.label = "abs. mean difference (price £)";

            vis.set_legend_visible(true);
            vis.set_axis_scale_types(vis::linear, vis::logarithmic);
            vis.set_axis_labels("iteration", "difference");
            vis.add(plot);
        }
        vis.end_frame();

        vis.run();
    }
};

TEST_F(BlackScholesUnitTest, TEMP)
{
    this->converges({
        .solver = black_scholes::explicit_scheme_solver,
        .solver_name = "explicit scheme solver",
        .time_steps = 10,
        .stock_price_steps = 3,
        .iterations = 100
    });

    this->converges({
        .solver = black_scholes::implicit_scheme_solver,
        .solver_name = "implicit scheme solver",
        .time_steps = 10,
        .stock_price_steps = 3,
        .iterations = 100
    });

    this->converges({
        .solver = static_cast<black_scholes::solution(*)(const black_scholes::input& input, const black_scholes::config& config)>(black_scholes::crank_nicholson_solver),
        .solver_name = "crank-nicholson solver",
        .time_steps = 10,
        .stock_price_steps = 3,
        .iterations = 100
    });
}
