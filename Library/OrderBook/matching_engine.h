#pragma once
#include <deque>
#include <filesystem>
#include <fstream>
#include <ranges>
#include "types.h"
#include "nlohmann/json.hpp"

class matching_engine
{
public:
    matching_engine() = delete;
    ~matching_engine() = default;

    matching_engine(const std::string& stock_name)
    : _stock_name(stock_name)
    {}

public:
    template <typename order_type>
    void match(order_type& order)
    {
        matching_engine::_match(order);
        matching_engine::clean_up();
    }

    void cancel(const uint32_t& user_id, const order_side& side, const uint32_t& order_id)
    {
        switch (side)
        {
            case SELL:
                for (auto& sell_orders : _sell_orders | std::views::values)
                    for (auto& sell_order : sell_orders)
                        if (sell_order.id == order_id)
                        {
                            matching_engine::remove(sell_order);
                            return;
                        }
            case BUY:
                for (auto& buy_orders : _buy_orders | std::views::values)
                    for (auto& buy_order : buy_orders)
                        if (buy_order.id == order_id)
                        {
                            matching_engine::remove(buy_order);
                            return;
                        }
                break;
        }
    }

private:
    void _match(limit_order<BUY>& buy_order)
    {
        // If there are no resting sell orders with the given price, put buy order in the book.
        if (!_sell_orders.contains(buy_order.price))
        {
            _buy_orders[buy_order.price].push_back(buy_order);
            return;
        }

        // Search for an exact match (same price and shares).
        auto& sell_orders = _sell_orders.at(buy_order.price);
        for (limit_order<SELL>& sell_order : sell_orders)
            if (sell_order.shares == buy_order.shares)
                if (matching_engine::complete_trade(buy_order, sell_order))
                    return;

        // If an exact match isn't found, consume cheapest sell offers until exhausted or buy order is complete.
        for (limit_order<SELL>& sell_order : sell_orders)
        {
            matching_engine::complete_trade(buy_order, sell_order);
            if (buy_order.shares == 0)
                return;
        }

        // If there are shares left over, put buy order in the book.
        _buy_orders[buy_order.price].push_back(buy_order);
    }
    void _match(limit_order<SELL>& sell_order)
    {
        // If there are no resting buy orders with the given price, put sell order in the book.
        if (!_buy_orders.contains(sell_order.price))
        {
            _sell_orders[sell_order.price].push_back(sell_order);
            return;
        }

        // Search for an exact match (same price and shares).
        auto& buy_orders = _buy_orders.at(sell_order.price);
        for (limit_order<BUY>& buy_order : buy_orders)
            if (buy_order.shares == sell_order.shares)
                if (matching_engine::complete_trade(buy_order, sell_order))
                    return;

        // If an exact match isn't found, consume best-paying buy offers until exhausted or sell order is complete.
        for (limit_order<BUY>& buy_order : buy_orders)
        {
            matching_engine::complete_trade(buy_order, sell_order);
            if (sell_order.shares == 0)
                return;
        }

        // If there are shares left over, put sell order in the book.
        _sell_orders[sell_order.price].push_back(sell_order);
    }
    void _match(market_order<BUY>& buy_order)
    {
        // Iterate over resting sell orders in order of lowest ask.
        for (auto& sell_orders: _sell_orders | std::views::values)
        {
            for (limit_order<SELL>& sell_order : sell_orders)
            {
                if (sell_order.user_id == buy_order.user_id)
                    continue;

                matching_engine::complete_trade(buy_order, sell_order);
                if (buy_order.shares == 0)
                    return;
            }
        }
    }
    void _match(market_order<SELL>& sell_order)
    {
        // Iterate over resting sell orders in order of lowest ask.
        for (auto& buy_orders: _buy_orders | std::views::values)
        {
            for (limit_order<BUY>& buy_order : buy_orders)
            {
                if (buy_order.user_id == sell_order.user_id)
                    continue;

                matching_engine::complete_trade(buy_order, sell_order);
                if (sell_order.shares == 0)
                    return;
            }
        }
    }

public:
    const auto& sell_orders() const { return _sell_orders; }
    const auto& buy_orders() const { return _buy_orders; }
    const auto& trades() const { return _trades; }

    std::string state_string() const
    {
        nlohmann::json x;
        x["completed_orders"] = _trades;
        x["buy_orders"] = _buy_orders;
        x["sell_orders"] = _sell_orders;
        return x.dump(2);
    }
    void log_state() const
    {
        std::filesystem::path output_path(OUTPUT_DIR);
        std::ofstream out_file(output_path/"state.json");
        out_file << state_string() << std::endl;
    }

private:
    bool complete_trade(limit_order<BUY>& buy_order,
                        limit_order<SELL>& sell_order)
    {
        if (buy_order.user_id == sell_order.user_id)
            return false;

        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.merchant_id = sell_order.user_id;
        trade.recipient_id = buy_order.user_id;
        trade.price = sell_order.price;
        trade.shares = std::min(sell_order.shares, buy_order.shares);
        trade.stock_name = sell_order.stock_name;

        buy_order.shares -= trade.shares;
        sell_order.shares -= trade.shares;
        if (buy_order.shares == 0) matching_engine::remove(buy_order);
        if (sell_order.shares == 0) matching_engine::remove(sell_order);

        matching_engine::complete_trade(trade);
        return true;
    }
    bool complete_trade(limit_order<BUY>& buy_order,
                        market_order<SELL>& sell_order)
    {
        if (buy_order.user_id == sell_order.user_id)
            return false;

        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.merchant_id = sell_order.user_id;
        trade.recipient_id = buy_order.user_id;
        trade.price = buy_order.price;
        trade.shares = std::min(sell_order.shares, buy_order.shares);
        trade.stock_name = sell_order.stock_name;

        buy_order.shares -= trade.shares;
        sell_order.shares -= trade.shares;
        if (buy_order.shares == 0) matching_engine::remove(buy_order);

        matching_engine::complete_trade(trade);
        return true;
    }
    bool complete_trade(market_order<BUY>& buy_order,
                        limit_order<SELL>& sell_order)
    {
        if (buy_order.user_id == sell_order.user_id)
            return false;

        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.merchant_id = sell_order.user_id;
        trade.recipient_id = buy_order.user_id;
        trade.price = sell_order.price;
        trade.shares = std::min(sell_order.shares, buy_order.shares);
        trade.stock_name = sell_order.stock_name;

        buy_order.shares -= trade.shares;
        sell_order.shares -= trade.shares;
        if (sell_order.shares == 0) matching_engine::remove(sell_order);

        matching_engine::complete_trade(trade);
        return true;
    }
    void complete_trade(trade& trade)
    {
        trade.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        _trades.push_back(trade);
        log_state();
    }

    void remove(limit_order<BUY>& buy_order)
    {
        if (!_buy_orders.contains(buy_order.price))
            return;

        auto& buy_orders = _buy_orders.at(buy_order.price);
        const auto it = std::ranges::find(buy_orders, buy_order);
        if (it == buy_orders.end()) return;

        const uint32_t price = buy_order.price;
        buy_orders.erase(it);
        if (buy_orders.empty())
            _empty_buy_order_price_lists.push_back(price);
    }
    void remove(limit_order<SELL>& sell_order)
    {
        if (!_sell_orders.contains(sell_order.price))
            return;

        auto& sell_orders = _sell_orders.at(sell_order.price);
        const auto it = std::ranges::find(sell_orders, sell_order);
        if (it == sell_orders.end()) return;

        const uint32_t price = sell_order.price;
        sell_orders.erase(it);
        if (sell_orders.empty())
            _empty_sell_order_price_lists.push_back(price);
    }
    void clean_up()
    {
        for (const auto& price : _empty_buy_order_price_lists)
            _buy_orders.erase(price);
        for (const auto& price : _empty_sell_order_price_lists)
            _sell_orders.erase(price);

        _empty_buy_order_price_lists.clear();
        _empty_sell_order_price_lists.clear();
    }

private:
    static uint32_t new_id(uint32_t& type_id) { return type_id++; }

private:
    uint32_t _completed_trade_id = 0;

    std::deque<uint32_t> _empty_buy_order_price_lists;
    std::deque<uint32_t> _empty_sell_order_price_lists;

    const std::string _stock_name;

    std::deque<trade> _trades;
    std::map<uint32_t, std::deque<limit_order<BUY>>, std::greater<>> _buy_orders;
    std::map<uint32_t, std::deque<limit_order<SELL>>, std::less<>> _sell_orders;
};
