#pragma once
// #include "gtest/gtest.h"
// #include "rapidcheck/gtest.h"
// #include "rapidcheck.h"
#include "order_book.h"

// class UnitTest : public ::testing::Test
// {
// public:
//     std::map<std::string, order_book> _order_books;
// };
//
// TEST_F(UnitTest, TMP)
// {
//     order_book book("AAPL");
//
//     book.ask(0, 500, 100'25);
//     book.ask(0, 100, 100'10);
//
//     book.bid(0, 60, 100'10);
//     book.bid(0, 100, 100'10);
// }
//
// TEST_F(UnitTest, ExactMatch)
// {
//     order_book book("AAPL");
//     client c0;
//     client c1;
//
//     // Place orders.
//     c0.ask(book, 200, 100'10);
//     c1.bid(book, 200, 100'10);
//
//     // Check trade has correct details.
//     auto trades = book.query_trades();
//     auto trade = trades.front();
//     ASSERT_EQ(trades.size(), 1);
//     ASSERT_EQ(trade.merchant_id, c0.id());
//     ASSERT_EQ(trade.recipient_id, c1.id());
//     ASSERT_EQ(trade.shares, 200);
//     ASSERT_EQ(trade.price, 100'10);
//     ASSERT_EQ(trade.stock_name, "AAPL");
//
//     // Ensure there are no buys/asks left.
//     auto buy_orders = book.query_resting_orders<BUY>();
//     auto sell_orders = book.query_resting_orders<SELL>();
//     ASSERT_TRUE(buy_orders.empty());
//     ASSERT_TRUE(sell_orders.empty());
// }
// TEST_F(UnitTest, LimitOrder_Bid)
// {
//     order_book book("AAPL");
//     client customer0;
//     client vendor0;
//     client vendor1;
//     client vendor2;
//
//     // Place orders.
//     vendor0.ask(book, 250, 100'10);
//     vendor1.ask(book, 200, 100'10);
//     vendor2.ask(book, 100, 100'10);
//     customer0.bid(book, 500, 100'10);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 250);
//     ASSERT_EQ(trades[0].price, 100'10);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor1.id());
//     ASSERT_EQ(trades[1].recipient_id, customer0.id());
//     ASSERT_EQ(trades[1].shares, 200);
//     ASSERT_EQ(trades[1].price, 100'10);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor2.id());
//     ASSERT_EQ(trades[2].recipient_id, customer0.id());
//     ASSERT_EQ(trades[2].shares, 50);
//     ASSERT_EQ(trades[2].price, 100'10);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     // Ensure there is a resting sell order from vendor 2 for the difference.
//     auto sell_orders = book.query_resting_orders<SELL>();
//     ASSERT_EQ(sell_orders.size(), 1);
//     ASSERT_EQ(sell_orders[0].user_id, vendor2.id());
//     ASSERT_EQ(sell_orders[0].shares, 50);
//     ASSERT_EQ(sell_orders[0].price, 100'10);
//     ASSERT_EQ(sell_orders[0].stock_name, "AAPL");
// }
// TEST_F(UnitTest, LimitOrder_Ask)
// {
//     order_book book("AAPL");
//     client vendor0;
//     client customer0;
//     client customer1;
//     client customer2;
//
//     // Place orders.
//     customer0.bid(book, 400, 100'10);
//     customer1.bid(book, 300, 100'10);
//     customer2.bid(book, 200, 100'10);
//     vendor0.ask(book, 750, 100'10);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 400);
//     ASSERT_EQ(trades[0].price, 100'10);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[1].recipient_id, customer1.id());
//     ASSERT_EQ(trades[1].shares, 300);
//     ASSERT_EQ(trades[1].price, 100'10);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[2].recipient_id, customer2.id());
//     ASSERT_EQ(trades[2].shares, 50);
//     ASSERT_EQ(trades[2].price, 100'10);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     // Ensure there is a resting buy order from customer 2 for the difference.
//     auto buy_orders = book.query_resting_orders<BUY>();
//     ASSERT_EQ(buy_orders.size(), 1);
//     ASSERT_EQ(buy_orders[0].user_id, customer2.id());
//     ASSERT_EQ(buy_orders[0].shares, 150);
//     ASSERT_EQ(buy_orders[0].price, 100'10);
//     ASSERT_EQ(buy_orders[0].stock_name, "AAPL");
// }
// TEST_F(UnitTest, MarketOrder_Bid_Fulfilled)
// {
//     order_book book("AAPL");
//     client vendor0;
//     client vendor1;
//     client vendor2;
//     client customer0;
//
//     // Place orders.
//     vendor0.ask(book, 100, 100'10);
//     vendor1.ask(book, 100, 100'25);
//     vendor2.ask(book, 100, 100'50);
//     customer0.bid(book, 250);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 100);
//     ASSERT_EQ(trades[0].price, 100'10);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor1.id());
//     ASSERT_EQ(trades[1].recipient_id, customer0.id());
//     ASSERT_EQ(trades[1].shares, 100);
//     ASSERT_EQ(trades[1].price, 100'25);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor2.id());
//     ASSERT_EQ(trades[2].recipient_id, customer0.id());
//     ASSERT_EQ(trades[2].shares, 50);
//     ASSERT_EQ(trades[2].price, 100'50);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     //  Ensure there is a resting sell order from vendor 2 for the difference.
//     auto sell_orders = book.query_resting_orders<SELL>();
//     ASSERT_EQ(sell_orders.size(), 1);
//     ASSERT_EQ(sell_orders[0].user_id, vendor2.id());
//     ASSERT_EQ(sell_orders[0].shares, 50);
//     ASSERT_EQ(sell_orders[0].price, 100'50);
//     ASSERT_EQ(sell_orders[0].stock_name, "AAPL");
// }
// TEST_F(UnitTest, MarketOrder_Bid_Underfilled)
// {
//     order_book book("AAPL");
//     client vendor0;
//     client vendor1;
//     client vendor2;
//     client customer0;
//
//     // Place orders.
//     vendor0.ask(book, 100, 100'10);
//     vendor1.ask(book, 100, 100'25);
//     vendor2.ask(book, 100, 100'50);
//     customer0.bid(book, 400);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 100);
//     ASSERT_EQ(trades[0].price, 100'10);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor1.id());
//     ASSERT_EQ(trades[1].recipient_id, customer0.id());
//     ASSERT_EQ(trades[1].shares, 100);
//     ASSERT_EQ(trades[1].price, 100'25);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor2.id());
//     ASSERT_EQ(trades[2].recipient_id, customer0.id());
//     ASSERT_EQ(trades[2].shares, 100);
//     ASSERT_EQ(trades[2].price, 100'50);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     //  Ensure there are no resting sell orders.
//     auto sell_orders = book.query_resting_orders<SELL>();
//     ASSERT_TRUE(sell_orders.empty());
// }
// TEST_F(UnitTest, MarketOrder_Ask_Fulfilled)
// {
//     order_book book("AAPL");
//     client customer0;
//     client customer1;
//     client customer2;
//     client vendor0;
//
//     // Place orders.
//     customer0.bid(book, 100, 100'50);
//     customer1.bid(book, 100, 100'25);
//     customer2.bid(book, 100, 100'10);
//     vendor0.ask(book, 250);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 100);
//     ASSERT_EQ(trades[0].price, 100'50);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[1].recipient_id, customer1.id());
//     ASSERT_EQ(trades[1].shares, 100);
//     ASSERT_EQ(trades[1].price, 100'25);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[2].recipient_id, customer2.id());
//     ASSERT_EQ(trades[2].shares, 50);
//     ASSERT_EQ(trades[2].price, 100'10);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     //  Ensure there is a resting sell order from vendor 2 for the difference.
//     auto buy_orders = book.query_resting_orders<BUY>();
//     ASSERT_EQ(buy_orders.size(), 1);
//     ASSERT_EQ(buy_orders[0].user_id, customer2.id());
//     ASSERT_EQ(buy_orders[0].shares, 50);
//     ASSERT_EQ(buy_orders[0].price, 100'10);
//     ASSERT_EQ(buy_orders[0].stock_name, "AAPL");
// }
// TEST_F(UnitTest, MarketOrder_Ask_Underfilled)
// {
//     order_book book("AAPL");
//     client customer0;
//     client customer1;
//     client customer2;
//     client vendor0;
//
//     // Place orders.
//     customer0.bid(book, 100, 100'50);
//     customer1.bid(book, 100, 100'25);
//     customer2.bid(book, 100, 100'10);
//     vendor0.ask(book, 350);
//
//     // Check trades have correct details.
//     auto trades = book.query_trades();
//     ASSERT_EQ(trades.size(), 3);
//     ASSERT_EQ(trades[0].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[0].recipient_id, customer0.id());
//     ASSERT_EQ(trades[0].shares, 100);
//     ASSERT_EQ(trades[0].price, 100'50);
//     ASSERT_EQ(trades[0].stock_name, "AAPL");
//     ASSERT_EQ(trades[1].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[1].recipient_id, customer1.id());
//     ASSERT_EQ(trades[1].shares, 100);
//     ASSERT_EQ(trades[1].price, 100'25);
//     ASSERT_EQ(trades[1].stock_name, "AAPL");
//     ASSERT_EQ(trades[2].merchant_id, vendor0.id());
//     ASSERT_EQ(trades[2].recipient_id, customer2.id());
//     ASSERT_EQ(trades[2].shares, 100);
//     ASSERT_EQ(trades[2].price, 100'10);
//     ASSERT_EQ(trades[2].stock_name, "AAPL");
//
//     //  Ensure there are no remaining buy orders.
//     auto buy_orders = book.query_resting_orders<BUY>();
//     ASSERT_TRUE(buy_orders.empty());
// }

// RC_GTEST_PROP(UnitTest, LimitOrder_Generated, ())
// {
//     const uint32_t shares = *rc::gen::inRange(100, 200);
//     const uint32_t price = *rc::gen::inRange(100'00, 200'00);
//     const order_side side = *rc::gen::element(BUY, SELL);
//
//
// }
