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
    template <order_like T>
    void match(T& order)
    {
        matching_engine::_match(order);
        matching_engine::clean_up();
    }

    void modify(const order_side& side, const id_type& order_id)
    {
        if (side == SELL)
        {

        }
        else
        {

        }
    }

    void cancel(const order_side& side, const id_type& order_id)
    {
        if (side == SELL)
        {
            for (auto& sell_orders : _sell_price_levels | std::views::values)
                for (auto& sell_order : sell_orders)
                    if (sell_order.id == order_id)
                    {
                        matching_engine::remove(sell_order);
                        return;
                    }
        }
        else
        {
            for (auto& buy_orders : _buy_price_levels | std::views::values)
                for (auto& buy_order : buy_orders)
                    if (buy_order.id == order_id)
                    {
                        matching_engine::remove(buy_order);
                        return;
                    }
        }
    }

private:
    /// Find resting sell orders to fill incoming limit buy order.
    template <incoming_order_like T>
    requires (T::type == LIMIT, T::side == BUY)
    auto find_sell_orders(const T& limit_buy_order, bool& sufficient_quantity)
    {
        std::deque<uint32_t> order_indices;
        const std::deque<resting_order<SELL>>& _sell_orders = _sell_price_levels.at(limit_buy_order.price);

        quantity_type remaining = limit_buy_order.quantity;
        for (uint32_t idx = 0; idx < _sell_orders.size(); ++idx)
        {
            if (remaining > 0)
            {
                order_indices.push_back(idx);
                remaining -= _sell_orders[idx].price;
            }
        }

        sufficient_quantity = remaining == 0;
        return order_indices;
    }

    // todo: copies, can optimise
    template <incoming_order_like T>
    void make_resting_order(const T& order)
    {
        if constexpr (T::side == BUY)
        {
            resting_order<BUY> resting_buy_order;
            resting_buy_order.id = order.id;
            resting_buy_order.price = order.price;
            resting_buy_order.quantity = order.quantity;
            resting_buy_order.good_until = 0;

            _buy_price_levels[order.price].push_back(resting_buy_order);
        }
        else
        {
            resting_order<SELL> resting_sell_order;
            resting_sell_order.id = order.id;
            resting_sell_order.price = order.price;
            resting_sell_order.quantity = order.quantity;
            resting_sell_order.good_until = 0;

            _sell_price_levels[order.price].push_back(resting_sell_order);
        }
    }

    template <incoming_order_like T>
    requires (T::type == LIMIT && T::side == BUY)
    void _match(T& limit_buy_order)
    {
        // If there are no resting sell orders with the given price, put buy order in the book.
        if (!_sell_price_levels.contains(limit_buy_order.price))
        {
            if constexpr (restable_order<T>)
                matching_engine::make_resting_order(limit_buy_order);

            return;
        }

        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto order_indices = matching_engine::find_sell_orders(limit_buy_order, sufficient_quantity);
        if constexpr (T::policy == FOK)
            if (!sufficient_quantity)
                return;

        // Consume cheapest sell offers until exhausted or buy order is complete.
        auto& sell_orders = _sell_price_levels.at(limit_buy_order.price);
        for (resting_order<SELL>& sell_order : sell_orders)
        {
            matching_engine::fill_order(limit_buy_order, sell_order);
            if (limit_buy_order.quantity == 0)
                return;
        }

        // If there are shares left over, put buy order in the book.
        if constexpr (restable_order<T>)
            matching_engine::make_resting_order(limit_buy_order);
    }
    template <incoming_order_like T>
    requires (T::type == LIMIT && T::side == SELL)
    void _match(T& limit_sell_order)
    {
        // If there are no resting buy orders with the given price, put sell order in the book.
        if (!_buy_price_levels.contains(limit_sell_order.price))
        {
            if constexpr (restable_order<T>)
                matching_engine::make_resting_order(limit_sell_order);

            return;
        }

        // Consume best-paying buy offers until exhausted or sell order is complete.
        auto& buy_orders = _buy_price_levels.at(limit_sell_order.price);
        for (resting_order<BUY>& buy_order : buy_orders)
        {
            matching_engine::fill_order(buy_order, limit_sell_order);
            if (limit_sell_order.quantity == 0)
                return;
        }

        // If there are shares left over, put sell order in the book.
        if constexpr (restable_order<T>)
            matching_engine::make_resting_order(limit_sell_order);
    }
    template <incoming_order_like T>
    requires (T::type == MARKET && T::side == BUY)
    void _match(T& market_buy_order)
    {
        // Iterate over resting sell orders in order of lowest ask.
        for (auto& sell_orders: _sell_price_levels | std::views::values)
        {
            for (resting_order<SELL>& sell_order : sell_orders)
            {
                matching_engine::fill_order(market_buy_order, sell_order);
                if (market_buy_order.quantity == 0)
                    return;
            }
        }
    }
    template <incoming_order_like T>
    requires (T::type == MARKET && T::side == SELL)
    void _match(T& market_sell_order)
    {
        // Iterate over resting sell orders in order of lowest ask.
        for (auto& buy_orders: _buy_price_levels | std::views::values)
        {
            for (resting_order<BUY>& buy_order : buy_orders)
            {
                matching_engine::fill_order(buy_order, market_sell_order);
                if (market_sell_order.quantity == 0)
                    return;
            }
        }
    }

public:
    const auto& sell_orders() const { return _sell_price_levels; }
    const auto& buy_orders() const { return _buy_price_levels; }
    const auto& trades() const { return _trades; }

    std::string state_string() const
    {
        nlohmann::json x;
        x["completed_orders"] = _trades;
        x["buy_price_levels"] = _buy_price_levels;
        x["sell_price_levels"] = _sell_price_levels;
        return x.dump(2);
    }
    void log_state() const
    {
        std::filesystem::path output_path(OUTPUT_DIR);
        std::ofstream out_file(output_path/"state.json");
        out_file << state_string() << std::endl;
    }

private:
    template <order_like T, order_like U>
    requires (T::side == BUY && U::side == SELL &&
              (resting_order_like<T> && limit_order_like<U>) ||
              (limit_order_like<T> && resting_order_like<U>))
    void fill_order(T& buy_order, U& sell_order)
    {
        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.ask_id = sell_order.id;
        trade.bid_id = buy_order.id;
        trade.price = sell_order.price;
        trade.quantity = std::min(sell_order.quantity, buy_order.quantity);

        buy_order.quantity -= trade.quantity;
        sell_order.quantity -= trade.quantity;
        if constexpr (resting_order_like<T>)
            if (buy_order.quantity == 0) matching_engine::remove(buy_order);
        if constexpr (resting_order_like<U>)
            if (sell_order.quantity == 0) matching_engine::remove(sell_order);

        matching_engine::fill_order(trade);
    }
    template <resting_order_like T, market_order_like U>
    requires (T::side == BUY && U::side == SELL)
    void fill_order(T& buy_order, U& sell_order)
    {
        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.ask_id = sell_order.id;
        trade.bid_id = buy_order.id;
        trade.price = buy_order.price;
        trade.quantity = std::min(sell_order.quantity, buy_order.quantity);

        buy_order.quantity -= trade.quantity;
        sell_order.quantity -= trade.quantity;
        if (buy_order.quantity == 0) matching_engine::remove(buy_order);

        matching_engine::fill_order(trade);
    }
    template <market_order_like T, resting_order_like U>
    requires (T::side == BUY && U::side == SELL)
    void fill_order(T& buy_order, U& sell_order)
    {
        trade trade;
        trade.id = matching_engine::new_id(_completed_trade_id);
        trade.ask_id = sell_order.id;
        trade.bid_id = buy_order.id;
        trade.price = sell_order.price;
        trade.quantity = std::min(sell_order.quantity, buy_order.quantity);

        buy_order.quantity -= trade.quantity;
        sell_order.quantity -= trade.quantity;
        if (sell_order.quantity == 0) matching_engine::remove(sell_order);

        matching_engine::fill_order(trade);
    }
    void fill_order(trade& trade)
    {
        trade.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        _trades.push_back(trade);
        log_state();
    }

    void remove(resting_order<BUY>& buy_order)
    {
        if (!_buy_price_levels.contains(buy_order.price))
            return;

        auto& buy_orders = _buy_price_levels.at(buy_order.price);
        const auto it = std::ranges::find(buy_orders, buy_order);
        if (it == buy_orders.end()) return;

        const uint32_t price = buy_order.price;
        buy_orders.erase(it);
        if (buy_orders.empty())
            _empty_buy_order_price_levels.push_back(price);
    }
    void remove(const resting_order<SELL>& sell_order)
    {
        if (!_sell_price_levels.contains(sell_order.price))
            return;

        auto& sell_orders = _sell_price_levels.at(sell_order.price);
        const auto it = std::ranges::find(sell_orders, sell_order);
        if (it == sell_orders.end()) return;

        const uint32_t price = sell_order.price;
        sell_orders.erase(it);
        if (sell_orders.empty())
            _empty_sell_order_price_levels.push_back(price);
    }
    void clean_up()
    {
        for (const auto& price : _empty_buy_order_price_levels)
            _buy_price_levels.erase(price);
        for (const auto& price : _empty_sell_order_price_levels)
            _sell_price_levels.erase(price);

        _empty_buy_order_price_levels.clear();
        _empty_sell_order_price_levels.clear();
    }

private:
    static uint32_t new_id(uint32_t& type_id) { return type_id++; }

private:
    const std::string _stock_name;

    std::deque<uint32_t> _empty_buy_order_price_levels;
    std::deque<uint32_t> _empty_sell_order_price_levels;

    uint32_t _completed_trade_id = 0;
    std::deque<trade> _trades;
    std::map<uint32_t, std::deque<resting_order<BUY>>, std::greater<>> _buy_price_levels;
    std::map<uint32_t, std::deque<resting_order<SELL>>, std::less<>>  _sell_price_levels;
};
