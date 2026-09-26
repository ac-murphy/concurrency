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
    matching_engine() = default;
    ~matching_engine() = default;

public:
    void match(limit_order<BUY>& buy_order)
    {
        // Ensure requested stock is available.
        if (!_sell_orders.contains(buy_order.stock_name))
        {
            std::cerr << "requested stock is unavailable" << std::endl;
            return;
        }

        // If there are no resting sell orders with the given price, put buy order in the book.
        if (!_sell_orders.at(buy_order.stock_name).contains(buy_order.price))
        {
            _buy_orders.at(buy_order.stock_name)[buy_order.price].push_back(buy_order);
            return;
        }

        // Search for an exact match (same price and shares).
        auto& sell_orders = _sell_orders.at(buy_order.stock_name).at(buy_order.price);
        for (limit_order<SELL>& sell_order : sell_orders)
            if (sell_order.shares == buy_order.shares)
            {
                matching_engine::complete_trade(buy_order, sell_order);
                return;
            }

        // If an exact match isn't found, consume cheapest sell offers until exhausted or buy order is complete.
        for (limit_order<SELL>& sell_order : sell_orders)
        {
            matching_engine::complete_trade(buy_order, sell_order);
            if (buy_order.shares == 0)
                return;
        }

        // If there are shares left over, put buy order in the book.
        _buy_orders.at(buy_order.stock_name)[buy_order.price].push_back(buy_order);
    }
    void match(limit_order<SELL>& sell_order)
    {
        // Ensure requested stock is available.
        if (!_sell_orders.contains(sell_order.stock_name))
        {
            std::cerr << "requested stock is unavailable" << std::endl;
            return;
        }

        // If there are no resting buy orders with the given price, put sell order in the book.
        if (!_buy_orders.at(sell_order.stock_name).contains(sell_order.price))
        {
            _sell_orders.at(sell_order.stock_name)[sell_order.price].push_back(sell_order);
            return;
        }

        // Search for an exact match (same price and shares).
        auto& buy_orders = _buy_orders.at(sell_order.stock_name).at(sell_order.price);
        for (limit_order<BUY>& buy_order : buy_orders)
            if (buy_order.shares == sell_order.shares)
            {
                matching_engine::complete_trade(buy_order, sell_order);
                return;
            }

        // If an exact match isn't found, consume best-paying buy offers until exhausted or sell order is complete.
        for (limit_order<BUY>& buy_order : buy_orders)
        {
            matching_engine::complete_trade(buy_order, sell_order);
            if (sell_order.shares == 0)
                return;
        }

        // If there are shares left over, put sell order in the book.
        _sell_orders.at(sell_order.stock_name)[sell_order.price].push_back(sell_order);
    }
    void match(market_order<BUY>& buy_order)
    {
        // Ensure requested stock is available.
        if (!_sell_orders.contains(buy_order.stock_name))
        {
            std::cerr << "requested stock is unavailable" << std::endl;
            return;
        }

        // Iterate over resting sell orders in order of lowest ask.
        for (auto& [price, sell_orders] : _sell_orders.at(buy_order.stock_name))
        {
            for (limit_order<SELL>& sell_order : sell_orders)
            {
                matching_engine::complete_trade(buy_order, sell_order);
                if (buy_order.shares == 0)
                    return;
            }
        }
    }
    void match(market_order<SELL>& sell_order)
    {
        // Ensure requested stock is available.
        if (!_buy_orders.contains(sell_order.stock_name))
        {
            std::cerr << "requested stock is unavailable" << std::endl;
            return;
        }

        // Iterate over resting sell orders in order of lowest ask.
        for (auto& [price, buy_orders] : _buy_orders.at(sell_order.stock_name))
        {
            for (limit_order<BUY>& buy_order : buy_orders)
            {
                matching_engine::complete_trade(buy_order, sell_order);
                if (sell_order.shares == 0)
                    return;
            }
        }
    }

    void new_stock(const stock& stock)
    {
        _buy_orders.try_emplace(stock.name);
        _sell_orders.try_emplace(stock.name);
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

    void complete_trade(limit_order<BUY>& buy_order,
                        limit_order<SELL>& sell_order)
    {
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
    }
    void complete_trade(limit_order<BUY>& buy_order,
                        market_order<SELL>& sell_order)
    {
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
    }
    void complete_trade(market_order<BUY>& buy_order,
                        limit_order<SELL>& sell_order)
    {
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
    }
    void complete_trade(const trade& trade)
    {
        _trades.push_back(trade);
        log_state();
    }

    void remove(limit_order<BUY>& buy_order)
    {
        if (!_buy_orders.at(buy_order.stock_name).contains(buy_order.price))
            return;

        auto& buy_orders = _buy_orders.at(buy_order.stock_name).at(buy_order.price);
        const auto it = std::ranges::find(buy_orders, buy_order);
        if (it == buy_orders.end())
            return;

        buy_orders.erase(it);
    }
    void remove(limit_order<SELL>& sell_order)
    {
        if (!_sell_orders.at(sell_order.stock_name).contains(sell_order.price))
            return;

        auto& sell_orders = _sell_orders.at(sell_order.stock_name).at(sell_order.price);
        const auto it = std::ranges::find(sell_orders, sell_order);
        if (it == sell_orders.end())
            return;

        sell_orders.erase(it);
    }

private:
    static uint32_t new_id(uint32_t& type_id) { return type_id++; }

private:
    uint32_t _completed_trade_id = 0;

    std::deque<trade> _trades;
    std::unordered_map<std::string, std::map<uint32_t, std::deque<limit_order<BUY>>, std::greater<>>> _buy_orders;
    std::unordered_map<std::string, std::map<uint32_t, std::deque<limit_order<SELL>>, std::less<>>> _sell_orders;
};
