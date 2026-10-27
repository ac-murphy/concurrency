#pragma once

#include <random>

#include "gtest/gtest.h"
#include "OrderBook/order_book.h"

class OrderBookSimulation : public ::testing::Test
{
protected:
    // Number of events in the simulation.
    static constexpr int N_events = 100;

    // Number of participants in the simulation.
    static constexpr int N_clients = 20;

    // Number of available actions at each event:
    //  - limit ask,
    //  - limit bid,
    //  - market ask,
    //  - market bid.
    static constexpr int N_actions = 4;

    // Initial values.
    static constexpr price_type initial_bid_price = 90'00;
    static constexpr price_type initial_ask_price = 100'00;

    // Distribution of random values.
    static constexpr quantity_type mean_quantity = 10;
    static constexpr quantity_type std_dev_quantity = 5;
    static constexpr quantity_type std_dev_ask_price = 5;
    static constexpr quantity_type std_dev_bid_price = 5;

protected:
    template <typename value_type>
    static value_type random_uniform_int(const value_type& min, const value_type& max)
    {
        static std::mt19937 gen;
        return std::uniform_int_distribution<value_type>(min, max)(gen);
    }
    template <typename value_type>
    static value_type random_normal_int(const value_type& mean, const value_type& std_deviation)
    {
        static std::mt19937 gen;
        return static_cast<value_type>(std::normal_distribution(static_cast<float>(mean), static_cast<float>(std_deviation))(gen));
    }

protected:
    static id_type random_client_id() { return random_uniform_int<id_type>(0, N_clients - 1); }
    static id_type random_action_id() { return random_uniform_int<id_type>(0, N_actions - 1); }
    static price_type random_ask_price(const order_book& book) { return random_normal_int<price_type>(book.sell_orders().empty() ? initial_ask_price : book.best_ask(), std_dev_ask_price); }
    static price_type random_bid_price(const order_book& book) { return random_normal_int<price_type>( book.buy_orders().empty() ? initial_bid_price : book.best_bid(), std_dev_bid_price); }
    static quantity_type random_quantity() { return random_normal_int<quantity_type>(mean_quantity, std_dev_quantity); }
    static order_policy random_policy() { return order_policy::GTC; }
};

TEST_F(OrderBookSimulation, Basic)
{
    struct book_state
    {
        std::vector<price_type> best_asks;
        std::vector<price_type> best_bids;
        std::vector<time_type> timestamps;
    };

    book_state state;
    time_type timestamp;

    // Initialise.
    order_book AAPL("AAPL");
    std::vector<client> clients(N_clients);

    // Perform actions.
    for (auto i = 0; i < N_events; ++i)
    {
        client& client = clients[random_client_id()];

        const auto action_id = random_action_id();
        const auto quantity = random_quantity();
        const auto ask_price = random_ask_price(AAPL);
        const auto bid_price = random_bid_price(AAPL);
        const auto policy = random_policy();
        switch (action_id)
        {
            case 0: client.limit_ask(AAPL, quantity, ask_price, policy); break;
            case 1: client.limit_bid(AAPL, quantity, bid_price, policy); break;
            case 2: client.market_ask(AAPL, quantity, policy); break;
            case 3: client.market_bid(AAPL, quantity, policy); break;
            default: throw std::logic_error(std::format("OrderBookSimulation: \"invalid action id: {}\"", action_id));
        }

        // Capture data.
        state.best_asks.push_back(AAPL.best_ask() == invalid_price ? std::numeric_limits<price_type>::quiet_NaN() : AAPL.best_ask());
        state.best_bids.push_back(AAPL.best_bid() == invalid_price ? std::numeric_limits<price_type>::quiet_NaN() : AAPL.best_bid());
        state.timestamps.push_back(timestamp++);
    }

    // Visualise.
    vis::visualise_plotting vis;
    vis.begin_frame();
    {
        vis::plot_2D<int32_t> plot_asks;
        plot_asks.x_values = state.timestamps;
        plot_asks.y_values = state.best_asks;
        vis.add_plot(plot_asks);

        vis::plot_2D<int32_t> plot_bids;
        plot_bids.x_values = state.timestamps;
        plot_bids.y_values = state.best_bids;
        vis.add_plot(plot_bids);
    }
    vis.end_frame();
    vis.run();
}