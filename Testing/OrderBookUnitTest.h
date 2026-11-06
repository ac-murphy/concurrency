#pragma once
#include <random>

#include "gtest/gtest.h"
#include "visualise.h"
#include "OrderBook/order_book.h"

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
    auto ask0 = c0.limit_ask(book, 200, 100'10);
    auto bid0 = c1.limit_bid(book, 200, 100'10);
    book.flush_log(true);

    // Check trade has correct details.
    auto trades = book.query_trades();
    auto trade = trades.front();
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trade.ask_id, ask0);
    ASSERT_EQ(trade.bid_id, bid0);
    ASSERT_EQ(trade.quantity, 200);
    ASSERT_EQ(trade.price, 100'10);

    // Ensure there are no buys/asks left.
    auto buy_orders = book.current_buy_orders();
    auto sell_orders = book.current_sell_orders();
    ASSERT_TRUE(buy_orders.empty());
    ASSERT_TRUE(sell_orders.empty());
}
TEST_F(UnitTest, LimitOrder_Bid_Fulfilled_GTC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 500, 100'10);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Bid_CrossesSpread_GTC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 200, 100'00, GTC);
    auto ask0 = vendor0.limit_ask(book, 100, 90'00, GTC);
    book.flush_log(true);

    // Check trade has correct details.
    auto trades = book.load_trades();
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'00);

    // Ensure there is a resting buy order from customer 0 for the difference.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid0);
    ASSERT_EQ(buy_orders[0].quantity, 100);
    ASSERT_EQ(buy_orders[0].price, 100'00);
}
TEST_F(UnitTest, LimitOrder_Ask_CrossesSpread_GTC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 90'00, GTC);
    auto bid0 = customer0.limit_bid(book, 200, 100'00, GTC);
    book.flush_log(true);

    // Check trade has correct details.
    auto trades = book.load_trades();
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 90'00);

    // Ensure there is a resting buy order from customer 0 for the difference.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid0);
    ASSERT_EQ(buy_orders[0].quantity, 100);
    ASSERT_EQ(buy_orders[0].price, 100'00);
}
TEST_F(UnitTest, LimitOrder_Bid_NoMatch_GTC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 110'00, GTC);
    auto bid0 = customer0.limit_bid(book, 200, 100'00, GTC);
    book.flush_log(true);

    // Check trade has correct details.
    auto trades = book.load_trades();
    ASSERT_EQ(trades.size(), 0);

    // Check incoming bid rests on book.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid0);
    ASSERT_EQ(buy_orders[0].quantity, 200);
    ASSERT_EQ(buy_orders[0].price, 100'00);

    // Ensure original ask is unchanged.
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask0);
    ASSERT_EQ(sell_orders[0].quantity, 100);
    ASSERT_EQ(sell_orders[0].price, 110'00);

}
TEST_F(UnitTest, LimitOrder_Bid_Underfilled_GTC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 600, 100'10);
    book.flush_log(true);

    // Check trades have correct details.
    auto trades = book.load_trades();
    // auto trades = book.query_trades();
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
    ASSERT_EQ(trades[2].quantity, 100);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure the difference for bid0 is resting on the book.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid0);
    ASSERT_EQ(buy_orders[0].quantity, 50);
    ASSERT_EQ(buy_orders[0].price, 100'10);

    // Ensure no sell orders remain.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
}
TEST_F(UnitTest, LimitOrder_Ask_Fulfilled_GTC)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 750, 100'10);
    book.flush_log(true);

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
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid2);
    ASSERT_EQ(buy_orders[0].quantity, 150);
    ASSERT_EQ(buy_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Ask_Underfilled_GTC)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 950, 100'10);
    book.flush_log(true);

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
    ASSERT_EQ(trades[2].quantity, 200);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure there is a resting sell order from vendor 0 for the difference.
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask0);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'10);

    // Ensure no buy orders remain.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Bid_Fulfilled_GTC)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 250);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'50);
}
TEST_F(UnitTest, MarketOrder_Bid_Underfilled_GTC)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 400);
    book.flush_log(true);

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

    // Ensure there are no resting sell orders.
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 0);

    // Ensure no buy orders were placed on book.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Ask_Fulfilled_GTC)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 250);
    book.flush_log(true);

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

    // Ensure there is a resting sell order from vendor 2 for the difference.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid2);
    ASSERT_EQ(buy_orders[0].quantity, 50);
    ASSERT_EQ(buy_orders[0].price, 100'10);

    // Ensure no sell order was placed on book.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Ask_Underfilled_GTC)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 350);
    book.flush_log(true);

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

    // Ensure there are no remaining buy orders.
    ASSERT_TRUE(book.current_buy_orders().empty());

    // Ensure difference for ask0 is cancelled.
    ASSERT_TRUE(book.current_sell_orders().empty());
}

TEST_F(UnitTest, LimitOrder_Bid_Fulfilled_FOK)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 500, 100'10, FOK);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Bid_Underfilled_FOK)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 600, 100'10, FOK);
    book.flush_log(true);

    // Ensure no trades have been made.
    ASSERT_EQ(book.query_trades().size(), 0);
    // Ensure sell orders have not been filled.
    ASSERT_EQ(book.current_sell_orders().size(), 3);
    // Ensure no buy orders have been placed.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, LimitOrder_Ask_Fulfilled_FOK)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 750, 100'10, FOK);
    book.flush_log(true);

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
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].quantity, 150);
    ASSERT_EQ(buy_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Ask_Underfilled_FOK)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 950, 100'10, FOK);
    book.flush_log(true);

    // Ensure no trades have been made.
    ASSERT_EQ(book.query_trades().size(), 0);
    // Ensure no sell orders have been placed.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
    // Ensure buy orders have not been filled.
    ASSERT_EQ(book.current_buy_orders().size(), 3);
}
TEST_F(UnitTest, MarketOrder_Bid_Fulfilled_FOK)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 250, FOK);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'50);

    // Ensure no buy orders remain.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Bid_Underfilled_FOK)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 400, FOK);
    book.flush_log(true);

    // Ensure no trades took place.
    ASSERT_EQ(book.query_trades().size(), 0);

    // Ensure resting sell orders remain.
    ASSERT_EQ(book.current_sell_orders().size(), 3);

    //  Ensure there are no resting buy orders.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Ask_Fulfilled_FOK)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 250, FOK);
    book.flush_log(true);

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
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid2);
    ASSERT_EQ(buy_orders[0].quantity, 50);
    ASSERT_EQ(buy_orders[0].price, 100'10);
}
TEST_F(UnitTest, MarketOrder_Ask_Underfilled_FOK)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 350, FOK);
    book.flush_log(true);

    // Ensure no trades took place.
    ASSERT_EQ(book.query_trades().size(), 0);

    //  Ensure there are no resting sell orders.
    ASSERT_EQ(book.current_sell_orders().size(), 0);

    // Ensure resting sell buy remain.
    ASSERT_EQ(book.current_buy_orders().size(), 3);
}
TEST_F(UnitTest, LimitOrder_Bid_Fulfilled_IOC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 500, 100'10, IOC);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Bid_Underfilled_IOC)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;
    client vendor1;
    client vendor2;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 250, 100'10);
    auto ask1 = vendor1.limit_ask(book, 200, 100'10);
    auto ask2 = vendor2.limit_ask(book, 100, 100'10);
    auto bid0 = customer0.limit_bid(book, 600, 100'10, IOC);
    book.flush_log(true);

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
    ASSERT_EQ(trades[2].quantity, 100);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure there are no sell orders.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
    // Ensure difference on bid is cancelled.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, LimitOrder_Ask_Fulfilled_IOC)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 750, 100'10, IOC);
    book.flush_log(true);

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

    // Ensure remaining difference on sell order is cancelled.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
    // Ensure no buy orders remain.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid2);
    ASSERT_EQ(buy_orders[0].quantity, 150);
    ASSERT_EQ(buy_orders[0].price, 100'10);
}
TEST_F(UnitTest, LimitOrder_Ask_Underfilled_IOC)
{
    order_book book("AAPL");
    client vendor0;
    client customer0;
    client customer1;
    client customer2;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 400, 100'10);
    auto bid1 = customer1.limit_bid(book, 300, 100'10);
    auto bid2 = customer2.limit_bid(book, 200, 100'10);
    auto ask0 = vendor0.limit_ask(book, 950, 100'10, IOC);
    book.flush_log(true);

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
    ASSERT_EQ(trades[2].quantity, 200);
    ASSERT_EQ(trades[2].price, 100'10);

    // Ensure remaining difference on sell order is cancelled.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
    // Ensure no buy orders remain.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Bid_Fulfilled_IOC)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 250, IOC);
    book.flush_log(true);

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
    auto sell_orders = book.current_sell_orders();
    ASSERT_EQ(sell_orders.size(), 1);
    ASSERT_EQ(sell_orders[0].id, ask2);
    ASSERT_EQ(sell_orders[0].quantity, 50);
    ASSERT_EQ(sell_orders[0].price, 100'50);
}
TEST_F(UnitTest, MarketOrder_Bid_Underfilled_IOC)
{
    order_book book("AAPL");
    client vendor0;
    client vendor1;
    client vendor2;
    client customer0;

    // Place orders.
    auto ask0 = vendor0.limit_ask(book, 100, 100'10);
    auto ask1 = vendor1.limit_ask(book, 100, 100'25);
    auto ask2 = vendor2.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.market_bid(book, 400, IOC);
    book.flush_log(true);

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

    // Ensure there are no resting sell orders.
    ASSERT_EQ(book.current_sell_orders().size(), 0);

    // Ensure no buy orders were placed on book.
    ASSERT_EQ(book.current_buy_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Ask_Fulfilled_IOC)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 250, IOC);
    book.flush_log(true);

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

    // Ensure there is a resting sell order from vendor 2 for the difference.
    auto buy_orders = book.current_buy_orders();
    ASSERT_EQ(buy_orders.size(), 1);
    ASSERT_EQ(buy_orders[0].id, bid2);
    ASSERT_EQ(buy_orders[0].quantity, 50);
    ASSERT_EQ(buy_orders[0].price, 100'10);

    // Ensure no sell order was placed on book.
    ASSERT_EQ(book.current_sell_orders().size(), 0);
}
TEST_F(UnitTest, MarketOrder_Ask_Underfilled_IOC)
{
    order_book book("AAPL");
    client customer0;
    client customer1;
    client customer2;
    client vendor0;

    // Place orders.
    auto bid0 = customer0.limit_bid(book, 100, 100'50);
    auto bid1 = customer1.limit_bid(book, 100, 100'25);
    auto bid2 = customer2.limit_bid(book, 100, 100'10);
    auto ask0 = vendor0.market_ask(book, 350, IOC);
    book.flush_log(true);

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

    // Ensure there are no remaining buy orders.
    ASSERT_TRUE(book.current_buy_orders().empty());

    // Ensure difference for ask0 is cancelled.
    ASSERT_TRUE(book.current_sell_orders().empty());
}
TEST_F(UnitTest, LimitOrder_Cancel)
{
    order_book book("AAPL");
    client customer0;
    client vendor0;

    auto ask0 = vendor0.limit_ask(book, 100, 100'50);
    auto bid0 = customer0.limit_bid(book, 100, 100'25);
    book.flush_log(true);

    // Initial state.
    ASSERT_EQ(book.query_trades().size(), 0);
    ASSERT_EQ(book.current_buy_orders().size(), 1);
    ASSERT_EQ(book.current_sell_orders().size(), 1);

    // After buy order cancel.
    bool successful = customer0.cancel_bid(book, bid0);
    ASSERT_TRUE(successful);
    ASSERT_EQ(book.query_trades().size(), 0);
    ASSERT_EQ(book.current_buy_orders().size(), 0);
    ASSERT_EQ(book.current_sell_orders().size(), 1);

    // Attempt to cancel made up buy order.
    successful = customer0.cancel_bid(book, 111);
    ASSERT_FALSE(successful);
    ASSERT_EQ(book.query_trades().size(), 0);
    ASSERT_EQ(book.current_buy_orders().size(), 0);
    ASSERT_EQ(book.current_sell_orders().size(), 1);

    // After sell order cancel.
    successful = customer0.cancel_ask(book, ask0);
    ASSERT_TRUE(successful);
    ASSERT_EQ(book.query_trades().size(), 0);
    ASSERT_EQ(book.current_buy_orders().size(), 0);
    ASSERT_EQ(book.current_sell_orders().size(), 0);

    // Attempt to cancel made up sell order.
    successful = customer0.cancel_ask(book, 111);
    ASSERT_FALSE(successful);
    ASSERT_EQ(book.query_trades().size(), 0);
    ASSERT_EQ(book.current_buy_orders().size(), 0);
    ASSERT_EQ(book.current_sell_orders().size(), 0);
}
TEST_F(UnitTest, CrossOrderDisabled)
{
    matching_config matching_config;
    matching_config.cross_orders_allowed = false;

    order_book AAPL("AAPL");
    AAPL.set_matching_config(matching_config);

    client user0;

    // Place orders.
    auto bid0 = user0.limit_bid(AAPL, 100, 100'50, GTC);
    auto ask0 = user0.limit_ask(AAPL, 350, 100'50, GTC);
    AAPL.flush_log(true);

    // Check trades have correct details.
    auto trades = AAPL.query_trades();
    ASSERT_EQ(trades.size(), 0);

    // Ensure bid is resting on book.
    auto bids = AAPL.current_buy_orders();
    ASSERT_EQ(bids.size(), 1);
    ASSERT_EQ(bids[0].id, bid0);
    ASSERT_EQ(bids[0].user_id, user0.id());
    ASSERT_EQ(bids[0].quantity, 100);
    ASSERT_EQ(bids[0].price, 100'50);

    // Ensure ask is resting on book.
    auto asks = AAPL.current_sell_orders();
    ASSERT_EQ(asks.size(), 1);
    ASSERT_EQ(asks[0].id, ask0);
    ASSERT_EQ(asks[0].user_id, user0.id());
    ASSERT_EQ(asks[0].quantity, 350);
    ASSERT_EQ(asks[0].price, 100'50);
}
TEST_F(UnitTest, CrossOrderEnabled)
{
    matching_config matching_config;
    matching_config.cross_orders_allowed = true;

    order_book AAPL("AAPL");
    AAPL.set_matching_config(matching_config);

    client user0;

    // Place orders.
    auto bid0 = user0.limit_bid(AAPL, 100, 100'50, GTC);
    auto ask0 = user0.limit_ask(AAPL, 350, 100'50, GTC);
    AAPL.flush_log(true);

    // Check trades have correct details.
    auto trades = AAPL.query_trades();
    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 100);
    ASSERT_EQ(trades[0].price, 100'50);

    // Ensure no bids remain.
    auto bids = AAPL.current_buy_orders();
    ASSERT_EQ(bids.size(), 0);

    // Ensure ask is resting on book.
    auto asks = AAPL.current_sell_orders();
    ASSERT_EQ(asks.size(), 1);
    ASSERT_EQ(asks[0].id, ask0);
    ASSERT_EQ(asks[0].user_id, user0.id());
    ASSERT_EQ(asks[0].quantity, 250);
    ASSERT_EQ(asks[0].price, 100'50);
}
TEST_F(UnitTest, InvalidOrders)
{
    market_config config = {
        .min_price = 100'00,
        .max_price = 150'00,
        .tick_size = 10,
        .min_quantity = 4,
        .max_quantity = 20,
        .lot_size = 2
    };

    order_book AAPL("AAPL");
    AAPL.set_market_config(config);

    client customer0;
    client vendor0;

    // Place valid orders.
    auto ask0 = vendor0.limit_ask(AAPL, 4, 100'00);
    auto ask1 = vendor0.limit_ask(AAPL, 8, 125'00);
    auto ask2 = vendor0.limit_ask(AAPL, 20, 150'00);
    auto bid0 = customer0.limit_bid(AAPL, 4, 100'10);
    auto bid1 = customer0.limit_bid(AAPL, 8, 125'10);
    auto bid2 = customer0.limit_bid(AAPL, 20, 149'90);

    // Place invalid orders (invalid price).
    auto ask3 = vendor0.limit_ask(AAPL, 10, 90'00);
    auto ask4 = vendor0.limit_ask(AAPL, 10, 200'00);
    auto ask5 = vendor0.limit_ask(AAPL, 10, 150'01);
    auto bid3 = customer0.limit_bid(AAPL, 10, 90'00);
    auto bid4 = customer0.limit_bid(AAPL, 10, 200'00);
    auto bid5 = customer0.limit_bid(AAPL, 10, 150'01);

    // Place invalid orders (invalid quantity).
    auto ask6 = vendor0.limit_ask(AAPL, 2, 150'00);
    auto ask7 = vendor0.limit_ask(AAPL, 22, 150'00);
    auto ask8 = vendor0.limit_ask(AAPL, 15, 150'00);
    auto bid6 = customer0.limit_bid(AAPL, 2, 150'00);
    auto bid7 = customer0.limit_bid(AAPL, 22, 150'00);
    auto bid8 = customer0.limit_bid(AAPL, 15, 150'00);

    AAPL.flush_log(true);

    // Check valid orders have valid IDs.
    ASSERT_NE(ask0, invalid_id);
    ASSERT_NE(ask1, invalid_id);
    ASSERT_NE(ask2, invalid_id);
    ASSERT_NE(bid0, invalid_id);
    ASSERT_NE(bid1, invalid_id);
    ASSERT_NE(bid2, invalid_id);

    // Check invalid orders have invalid IDs.
    ASSERT_EQ(ask3, invalid_id);
    ASSERT_EQ(ask4, invalid_id);
    ASSERT_EQ(ask5, invalid_id);
    ASSERT_EQ(ask6, invalid_id);
    ASSERT_EQ(ask7, invalid_id);
    ASSERT_EQ(ask8, invalid_id);
    ASSERT_EQ(bid3, invalid_id);
    ASSERT_EQ(bid4, invalid_id);
    ASSERT_EQ(bid5, invalid_id);
    ASSERT_EQ(bid6, invalid_id);
    ASSERT_EQ(bid7, invalid_id);
    ASSERT_EQ(bid8, invalid_id);

    // Check trades have correct details.
    auto trades = AAPL.query_trades();
    ASSERT_EQ(trades.size(), 2);
    ASSERT_EQ(trades[0].ask_id, ask0);
    ASSERT_EQ(trades[0].bid_id, bid0);
    ASSERT_EQ(trades[0].quantity, 4);
    ASSERT_EQ(trades[0].price, 100'00);
    ASSERT_EQ(trades[1].ask_id, ask1);
    ASSERT_EQ(trades[1].bid_id, bid1);
    ASSERT_EQ(trades[1].quantity, 8);
    ASSERT_EQ(trades[1].price, 125'00);

    // Ensure there is a resting sell order.
    auto asks = AAPL.current_sell_orders();
    ASSERT_EQ(asks.size(), 1);
    ASSERT_EQ(asks[0].id, ask2);
    ASSERT_EQ(asks[0].quantity, 20);
    ASSERT_EQ(asks[0].price, 150'00);

    // Ensure there is a resting buy order.
    auto bids = AAPL.current_buy_orders();
    ASSERT_EQ(bids.size(), 1);
    ASSERT_EQ(bids[0].id, bid2);
    ASSERT_EQ(bids[0].quantity, 20);
    ASSERT_EQ(bids[0].price, 149'90);
}

