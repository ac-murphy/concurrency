#pragma once
#include <random>

#include "gtest/gtest.h"
#include "order_book.h"
#include "visualise.h"

class UnitTest : public ::testing::Test
{
public:
    std::map<std::string, order_book> _order_books;
};

TEST_F(UnitTest, TMP)
{
    order_book book("AAPL");

    book.ask(500, 100'25);
    book.ask(100, 100'10);

    book.bid(60, 100'10);
    book.bid(100, 100'10);
}

TEST_F(UnitTest, ExactMatch)
{
    order_book book("AAPL");
    client c0;
    client c1;

    // Place orders.
    auto ask0 = c0.ask(book, 200, 100'10);
    auto bid0 = c1.bid(book, 200, 100'10);

    // Check trade has correct details.
    auto trades = book.query_trades();
    auto trade = trades.front();
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trade.ask_id, ask0);
    ASSERT_EQ(trade.bid_id, bid0);
    ASSERT_EQ(trade.quantity, 200);
    ASSERT_EQ(trade.price, 100'10);

    // Ensure there are no buys/asks left.
    auto buy_orders = book.query_resting_orders<BUY>();
    auto sell_orders = book.query_resting_orders<SELL>();
    ASSERT_TRUE(buy_orders.empty());
    ASSERT_TRUE(sell_orders.empty());
}
TEST_F(UnitTest, LimitOrder_Bid)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.ask(book, 250, 100'10);
    auto ask1 = vendor1.ask(book, 200, 100'10);
    auto ask2 = vendor2.ask(book, 100, 100'10);
    auto bid0 = customer0.bid(book, 500, 100'10);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 250);
    ASSERT_EQ(trades[0].price, 100'10);
    ASSERT_EQ(trades[1].ask_id, ask1);
    ASSERT_EQ(trades[1].bid_id, bid0);
    ASSERT_EQ(trades[1].quantity, 200);
    ASSERT_EQ(trades[1].price, 100'10);
    ASSERT_EQ(trades[2].ask_id, ask2);
    ASSERT_EQ(trades[2].bid_id, bid0);
    ASSERT_EQ(trades[2].quantity, 50);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure there is a resting sell order from vendor 2 for the difference.
    auto sell_orders = book.query_resting_orders<SELL>();
    ASSERT_EQ(sell_orders.size(), 1);
    // ASSERT_EQ(sell_orders[0].user_id, vendor2.id());
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'10);
    // ASSERT_EQ(sell_orders[0].stock_name, "AAPL");
}
TEST_F(UnitTest, LimitOrder_Ask)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.bid(book, 400, 100'10);
    auto bid1 = customer1.bid(book, 300, 100'10);
    auto bid2 = customer2.bid(book, 200, 100'10);
    auto ask0 = vendor0.ask(book, 750, 100'10);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 400);
    ASSERT_EQ(trades[0].price, 100'10);
    ASSERT_EQ(trades[1].ask_id, ask0);
    ASSERT_EQ(trades[1].bid_id, bid1);
    ASSERT_EQ(trades[1].quantity, 300);
    ASSERT_EQ(trades[1].price, 100'10);
    ASSERT_EQ(trades[2].ask_id, ask0);
    ASSERT_EQ(trades[2].bid_id, bid2);
    ASSERT_EQ(trades[2].quantity, 50);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure there is a resting buy order from customer 2 for the difference.
    auto buy_orders = book.query_resting_orders<BUY>();
    ASSERT_EQ(buy_orders.size(), 1);
    // ASSERT_EQ(buy_orders[0].user_id, customer2.id());
    ASSERT_EQ(buy_orders[0].quantity, 150);
    ASSERT_EQ(buy_orders[0].price, 100'10);
    // ASSERT_EQ(buy_orders[0].stock_name, "AAPL");
}
TEST_F(UnitTest, MarketOrder_Bid_Fulfilled)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.ask(book, 100, 100'10);
    auto ask1 = vendor1.ask(book, 100, 100'25);
    auto ask2 = vendor2.ask(book, 100, 100'50);
    auto bid0 = customer0.bid(book, 250);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'10);
    ASSERT_EQ(trades[1].ask_id, ask1);
    ASSERT_EQ(trades[1].bid_id, bid0);
    ASSERT_EQ(trades[1].quantity, 100);
    ASSERT_EQ(trades[1].price, 100'25);
    ASSERT_EQ(trades[2].ask_id, ask2);
    ASSERT_EQ(trades[2].bid_id, bid0);
    ASSERT_EQ(trades[2].quantity, 50);
    ASSERT_EQ(trades[2].price, 100'50);

    //  Ensure there is a resting sell order from vendor 2 for the difference.
    auto sell_orders = book.query_resting_orders<SELL>();
    ASSERT_EQ(sell_orders.size(), 1);
    // ASSERT_EQ(sell_orders[0].user_id, vendor2.id());
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'50);
    // ASSERT_EQ(sell_orders[0].stock_name, "AAPL");
}
TEST_F(UnitTest, MarketOrder_Bid_Underfilled)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.ask(book, 100, 100'10);
    auto ask1 = vendor1.ask(book, 100, 100'25);
    auto ask2 = vendor2.ask(book, 100, 100'50);
    auto bid0 = customer0.bid(book, 400);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'10);
    ASSERT_EQ(trades[1].ask_id, ask1);
    ASSERT_EQ(trades[1].bid_id, bid0);
    ASSERT_EQ(trades[1].quantity, 100);
    ASSERT_EQ(trades[1].price, 100'25);
    ASSERT_EQ(trades[2].ask_id, ask2);
    ASSERT_EQ(trades[2].bid_id, bid0);
    ASSERT_EQ(trades[2].quantity, 100);
    ASSERT_EQ(trades[2].price, 100'50);

    //  Ensure there are no resting sell orders.
    auto sell_orders = book.query_resting_orders<SELL>();
    ASSERT_TRUE(sell_orders.empty());
}
TEST_F(UnitTest, MarketOrder_Ask_Fulfilled)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.bid(book, 100, 100'50);
    auto bid1 = customer1.bid(book, 100, 100'25);
    auto bid2 = customer2.bid(book, 100, 100'10);
    auto ask0 = vendor0.ask(book, 250);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'50);
    ASSERT_EQ(trades[1].ask_id, ask0);
    ASSERT_EQ(trades[1].bid_id, bid1);
    ASSERT_EQ(trades[1].quantity, 100);
    ASSERT_EQ(trades[1].price, 100'25);
    ASSERT_EQ(trades[2].ask_id, ask0);
    ASSERT_EQ(trades[2].bid_id, bid2);
    ASSERT_EQ(trades[2].quantity, 50);
    ASSERT_EQ(trades[2].price, 100'10);

    //  Ensure there is a resting sell order from vendor 2 for the difference.
    auto buy_orders = book.query_resting_orders<BUY>();
    ASSERT_EQ(buy_orders.size(), 1);
    // ASSERT_EQ(buy_orders[0].user_id, customer2.id());
    ASSERT_EQ(buy_orders[0].quantity, 50);
    ASSERT_EQ(buy_orders[0].price, 100'10);
    // ASSERT_EQ(buy_orders[0].stock_name, "AAPL");
}
TEST_F(UnitTest, MarketOrder_Ask_Underfilled)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.bid(book, 100, 100'50);
    auto bid1 = customer1.bid(book, 100, 100'25);
    auto bid2 = customer2.bid(book, 100, 100'10);
    auto ask0 = vendor0.ask(book, 350);

    // Check trades have correct details.
    auto trades = book.query_trades();
    ASSERT_EQ(trades.size(), 3);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'50);
    ASSERT_EQ(trades[1].ask_id, ask0);
    ASSERT_EQ(trades[1].bid_id, bid1);
    ASSERT_EQ(trades[1].quantity, 100);
    ASSERT_EQ(trades[1].price, 100'25);
    ASSERT_EQ(trades[2].ask_id, ask0);
    ASSERT_EQ(trades[2].bid_id, bid2);
    ASSERT_EQ(trades[2].quantity, 100);
    ASSERT_EQ(trades[2].price, 100'10);

    //  Ensure there are no remaining buy orders.
    auto buy_orders = book.query_resting_orders<BUY>();
    ASSERT_TRUE(buy_orders.empty());
}
// TEST_F(UnitTest, Simulation)
// {
//     constexpr size_t N_clients = 10;
//     constexpr size_t N_random_orders = 10;
//     constexpr size_t N_orders = 100;
//     std::mt19937 rng(std::random_device{}());
//     std::uniform_int_distribution<int32_t> rand_client_idx(0, N_clients - 1);
//     std::uniform_real_distribution<float> rand_price_offset_factor(0.9f, 1.1f);
//     std::uniform_int_distribution<int32_t> rand_shares_count(50, 500);
//     std::uniform_real_distribution<float> rand_order_side(0.0f, 1.0f);
//
//     order_book book_AAPL("AAPL");
//     std::vector<client> clients(N_clients);
//
//     // for (size_t i = 0; i < N_random_orders; ++i)
//     // {
//     //     const client& c = clients[rand_client_idx(rng)];
//     //     const order_side side = rand_order_side(rng) > 0.3f ? BUY : SELL;
//     //     const float price_scale = rand_price_offset_factor(rng);
//     //     const uint32_t price_value = static_cast<uint32_t>(((book_AAPL.best_bid() + book_AAPL.best_ask()) / 2) * price_scale);
//     //     const uint32_t shares_count = rand_shares_count(rng);
//     //
//     //     switch (side)
//     //     {
//     //         case BUY:
//     //             c.bid(book_AAPL, shares_count, price_value);
//     //             break;
//     //         case SELL:
//     //             c.ask(book_AAPL, shares_count, price_value);
//     //             break;
//     //     }
//     // }
// }
// TEST_F(UnitTest, ManualSimulation)
// {
//     order_book book_AAPL("AAPL");
//     client c0, c1, c2, c3;
//
//     c0.ask(book_AAPL, 100, 100'10);
//     c1.ask(book_AAPL, 100, 100'25);
//     c2.ask(book_AAPL, 100, 100'10);
//     c3.bid(book_AAPL, 250);
//
//
// }
