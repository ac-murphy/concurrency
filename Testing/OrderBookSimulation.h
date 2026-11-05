#pragma once

#include <random>

#include "gtest/gtest.h"
#include "OrderBook/order_book.h"
#include "Tracy.hpp"

class OrderBookSimulation : public ::testing::Test
{
protected:
    // Number of events in the simulation.
    static constexpr int N_events = 10000000;

protected:
    template <typename int_type = int>
    static int_type random_uniform_int(const int_type& min, const int_type& max)
    {
        static std::mt19937 gen;
        return std::uniform_int_distribution<int_type>(min, max)(gen);
    }
    template <typename fp_type = float>
    static fp_type random_uniform_fp(const fp_type& min, const fp_type& max)
    {
        static std::mt19937 gen;
        return std::uniform_real_distribution<fp_type>(min, max)(gen);
    }
    template <typename int_type = int>
    static int_type random_normal_int(const int_type& mean, const int_type& std_deviation)
    {
        static std::mt19937 gen;
        return static_cast<int_type>(std::normal_distribution(static_cast<float>(mean), static_cast<float>(std_deviation))(gen));
    }

protected:
    static bool random_action() { return random_uniform_int(0, 5); }
    static quantity_type random_quantity() { return random_uniform_int<quantity_type>(1, 100); }
    static price_type ask_price(const order_book& book, const price_type& fair_price, const price_type& spread)
    {
        const price_type best_ask = book.best_ask();
        if (best_ask == invalid_price)
            return fair_price + (spread / 2);
        else
            return std::min(best_ask + random_normal_int(1, 4), fair_price);
    }
    static price_type bid_price(const order_book& book, const price_type& fair_price, const price_type& spread)
    {
        const price_type best_bid = book.best_bid();
        if (best_bid == invalid_price)
            return fair_price - (spread / 2);
        else
            return std::max(best_bid - random_normal_int(1, 4), fair_price);
    }
};

TEST_F(OrderBookSimulation, Basic)
{
    // struct book_state
    // {
    //     std::vector<price_type> fair_price;
    //     std::vector<price_type> best_asks, best_bids, spread, trade_prices;
    //     std::vector<time_type> timestamps;
    // };

    // book_state state;
    std::vector<price_type> fair_prices;
    time_type timestamp = 0;

    // Initialise.
    order_book AAPL("AAPL");

    client client;
    price_type baseline_price = 100'00;
    price_type fair_price = baseline_price;
    price_type spread = 10;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform actions.
    for (auto i = 0; i < N_events; ++i)
    {
#if TRACY_ENABLE == 1
        ZoneScopedN("loop");
#endif

        bool action;
        quantity_type quantity;
        {
#if TRACY_ENABLE == 1
            ZoneScopedN("generate sim. values");
#endif
            fair_price += random_normal_int(0, 1);
            fair_prices.push_back(fair_price);
            action = random_action();
            quantity = random_quantity();
        }

        if (action == 0)
        {
            if (random_uniform_int(0, 1) == 0)
                client.market_ask(AAPL, quantity, order_policy::GTC);
            else
                client.market_bid(AAPL, quantity, order_policy::GTC);
        }
        else
        {
            const auto n_orders = static_cast<float>(AAPL.resting_buy_order_count() + AAPL.resting_sell_order_count());
            const auto n_bids = static_cast<float>(AAPL.resting_buy_order_count());
            if (random_uniform_fp(0.0f, 1.0f) < n_bids / n_orders)
            {
                client.limit_ask(AAPL, 3 * quantity, ask_price(AAPL, fair_price, spread), order_policy::GTC);
            }
            else
                client.limit_bid(AAPL, 3 * quantity, bid_price(AAPL, fair_price, spread), order_policy::GTC);
        }

        // Capture data.
        {
            // state.best_asks.push_back(AAPL.best_ask());
            // state.best_bids.push_back(AAPL.best_bid());
            // state.trade_prices.push_back(AAPL.query_trades().empty() ? invalid_price : AAPL.query_trades().back().price);
            // state.spread.push_back(AAPL.best_ask() == invalid_price || AAPL.best_bid() == invalid_price ? invalid_price : AAPL.best_ask() - AAPL.best_bid());
            // state.timestamps.push_back(++timestamp);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;

    // Post-simulation capture.
    std::vector<price_type> trade_prices;
    std::vector<time_type> trade_timestamps;
    for (const trade& trade : AAPL.trades())
    {
        trade_prices.push_back(trade.price);
        trade_timestamps.push_back(trade.timestamp);
    }

    // Visualise.
    // vis::visualise_plotting vis;
    // vis.begin_frame();
    // {
    //     vis.set_legend_visible(true);
    //     vis.set_axis_labels("event", R"(price (£))");
    //
    //     vis::plot_2D<uint32_t> plot_asks;
    //     plot_asks.label = "best ask";
    //     plot_asks.x_values = state.timestamps;
    //     plot_asks.y_values = state.best_asks;
    //     vis.add(plot_asks);
    //
    //     vis::plot_2D<uint32_t> plot_bids;
    //     plot_bids.label = "best bid";
    //     plot_bids.x_values = state.timestamps;
    //     plot_bids.y_values = state.best_bids;
    //     vis.add(plot_bids);
    //
    //     vis::plot_2D<uint32_t> plot_fair_prices;
    //     plot_fair_prices.label = "fair price";
    //     plot_fair_prices.x_values = state.timestamps;
    //     plot_fair_prices.y_values = fair_prices;
    //     vis.add(plot_fair_prices);
    //
    //     vis::plot_2D<uint32_t> plot_trade_prices;
    //     plot_trade_prices.label = "trade price";
    //     plot_trade_prices.x_values = state.timestamps;
    //     plot_trade_prices.y_values = state.trade_prices;
    //     vis.add(plot_trade_prices);
    // }
    // vis.end_frame();
    //
    // vis.begin_frame();
    // {
    //     vis::plot_2D<uint32_t> plot_spread;
    //     plot_spread.label = "spread";
    //     plot_spread.x_values = state.timestamps;
    //     plot_spread.y_values = state.spread;
    //     vis.add(plot_spread);
    // }
    // vis.end_frame();

    // vis.begin_frame();
    // {
    //     vis.set_legend_visible(true);
    //     vis.set_axis_labels("time", R"(price (£))");
    //
    //     vis::plot_2D<uint32_t> plot_trade_prices;
    //     plot_trade_prices.label = "trade price";
    //     plot_trade_prices.x_values = trade_timestamps;
    //     plot_trade_prices.y_values = trade_prices;
    //     vis.add(plot_trade_prices);
    // }
    // vis.end_frame();

    // vis.run();
}