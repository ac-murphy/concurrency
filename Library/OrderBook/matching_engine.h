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
    void match(incoming_order& order)
    {
        if (order.type == LIMIT)
        {
            if (order.side == BUY)
                matching_engine::_match_limit_buy_order(order);
            if (order.side == SELL)
                matching_engine::_match_limit_sell_order(order);
        }
        if (order.type == MARKET)
        {
            if (order.side == BUY)
                matching_engine::_match_market_buy_order(order);
            if (order.side == SELL)
                matching_engine::_match_market_sell_order(order);
        }

        matching_engine::_clean_up();
    }
    bool cancel(const order_side& side, const id_type& order_id)
    {
        bool success = false;
        if (side == BUY)
            success = matching_engine::_cancel_impl(order_id, _buy_price_levels);
        if (side == SELL)
            success = matching_engine::_cancel_impl(order_id, _sell_price_levels);

        matching_engine::_clean_up();
        return success;
    }

public:
    void set_config(const matching_config& config)
    {
        _config = config;
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
    uint32_t _evaluate_trade_price(const T& buy_order, const U& sell_order)
    {
        if (is_fixed_price_order(buy_order) &&
            is_fixed_price_order(sell_order))
            return std::min(buy_order.price, sell_order.price);

        if (is_fixed_price_order(buy_order) &&
            is_variable_price_order(sell_order))
            return buy_order.price;

        if (is_variable_price_order(buy_order) &&
            is_fixed_price_order(sell_order))
            return sell_order.price;

        return std::numeric_limits<uint32_t>::max();
    }
    template <order_like T, order_like U>
    uint32_t _evaluate_trade_quantity(const T& buy_order, const U& sell_order)
    {
        return std::min(buy_order.quantity, sell_order.quantity);
    }

    void _make_resting_order(const incoming_order& incoming_order)
    {
        resting_order resting_order;
        resting_order.side = incoming_order.side;
        resting_order.quantity = incoming_order.quantity;
        resting_order.price = incoming_order.price;
        resting_order.id = incoming_order.id;

        // todo: implement properly.
        resting_order.good_until = 0;

        if (incoming_order.side == BUY)
            _buy_price_levels[incoming_order.price].push_back(resting_order);
        else
            _sell_price_levels[incoming_order.price].push_back(resting_order);
    }

    template <order_like T, order_like U>
    void _fill_order(T& buy_order, U& sell_order)
    {
        if (!(buy_order.side == order_side::BUY && sell_order.side == order_side::SELL))
        {
            std::cout << "hello'";
        }

        trade trade;
        trade.id = matching_engine::_new_id(_completed_trade_id);
        trade.ask_id = sell_order.id;
        trade.bid_id = buy_order.id;
        trade.price = matching_engine::_evaluate_trade_price(buy_order, sell_order);
        trade.quantity = matching_engine::_evaluate_trade_quantity(buy_order, sell_order);

        buy_order.quantity -= trade.quantity;
        sell_order.quantity -= trade.quantity;
        if constexpr (is_resting_order<T>)
            if (buy_order.quantity == 0)
                matching_engine::_remove_order(buy_order);
        if constexpr (is_resting_order<U>)
            if (sell_order.quantity == 0)
                matching_engine::_remove_order(sell_order);

        matching_engine::_complete_trade(trade);
    }
    void _complete_trade(trade& trade)
    {
        trade.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        _trades.push_back(trade);
        log_state();
    }
    void _match_limit_buy_order(incoming_order& limit_buy_order)
    {
        // If there are no resting sell orders with the given price, put buy order in the book.
        if (!_sell_price_levels.contains(limit_buy_order.price))
        {
            if (is_restable_order(limit_buy_order))
                matching_engine::_make_resting_order(limit_buy_order);

            return;
        }

        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto sell_order_pointers = matching_engine::_find_orders(limit_buy_order, sufficient_quantity);
        if (!is_partially_fillable_order(limit_buy_order))
            if (!sufficient_quantity)
                return;

        // Consume cheapest sell offers until exhausted or buy order is complete.
        // auto& sell_orders = _sell_price_levels.at(limit_buy_order.price);
        for (resting_order* sell_order_ptr : sell_order_pointers)
        {
            matching_engine::_fill_order(limit_buy_order, *sell_order_ptr);
            if (limit_buy_order.quantity == 0)
                return;
        }

        // If there are shares left over, put buy order in the book.
        if (is_restable_order(limit_buy_order))
            matching_engine::_make_resting_order(limit_buy_order);
    }
    void _match_limit_sell_order(incoming_order& limit_sell_order)
    {
        // If there are no resting buy orders with the given price, put sell order in the book.
        if (!_buy_price_levels.contains(limit_sell_order.price))
        {
            if (is_restable_order(limit_sell_order))
                matching_engine::_make_resting_order(limit_sell_order);

            return;
        }

        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto buy_order_pointers = matching_engine::_find_orders(limit_sell_order, sufficient_quantity);
        if (!is_partially_fillable_order(limit_sell_order))
            if (!sufficient_quantity)
                return;

        // Consume best-paying buy offers until exhausted or sell order is complete.
        for (resting_order* buy_order_ptr : buy_order_pointers)
        {
            matching_engine::_fill_order(*buy_order_ptr, limit_sell_order);
            if (limit_sell_order.quantity == 0)
                return;
        }

        // If there are shares left over, put sell order in the book.
        if (is_restable_order(limit_sell_order))
            matching_engine::_make_resting_order(limit_sell_order);
    }

    /* void _match_market_order_impl(incoming_order& incoming_order)
    {
        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto order_pointers = matching_engine::_find_orders(incoming_order, sufficient_quantity);
        if (!is_partially_fillable_order(incoming_order))
            if (!sufficient_quantity)
                return;

        // Iterate over resting sell orders in order of lowest ask.
        for (resting_order* order_ptr : order_pointers)
        {
            if (incoming_order.side == BUY)
                matching_engine::_fill_order(incoming_order, *order_ptr);
            if (incoming_order.side == SELL)
                matching_engine::_fill_order(*order_ptr, incoming_order);
            if (incoming_order.quantity == 0)
                return;
        }
    } */
    void _match_market_buy_order(incoming_order& market_buy_order)
    {
        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto sell_order_pointers = matching_engine::_find_orders(market_buy_order, sufficient_quantity);
        if (!is_partially_fillable_order(market_buy_order))
            if (!sufficient_quantity)
                return;

        // Iterate over resting sell orders in order of lowest ask.
        for (resting_order* sell_order_ptr : sell_order_pointers)
        {
            matching_engine::_fill_order(market_buy_order, *sell_order_ptr);
            if (market_buy_order.quantity == 0)
                return;
        }
    }
    void _match_market_sell_order(incoming_order& market_sell_order)
    {
        // Find relevant orders (by index) and check quantity when necessary.
        bool sufficient_quantity;
        auto buy_order_pointers = matching_engine::_find_orders(market_sell_order, sufficient_quantity);
        if (!is_partially_fillable_order(market_sell_order))
            if (!sufficient_quantity)
                return;

        // Iterate over resting sell orders in order of lowest ask.
        for (resting_order* buy_order_ptr : buy_order_pointers)
        {
            matching_engine::_fill_order(*buy_order_ptr, market_sell_order);
            if (market_sell_order.quantity == 0)
                return;
        }
    }

    template <typename PriceLevels>
    auto _find_orders_impl(const incoming_order& incoming_order,
                           PriceLevels& price_levels,
                           std::deque<resting_order*>& relevant_orders,
                           bool& has_sufficient_quantity) const
    {
        int32_t remaining_qty = incoming_order.quantity;

        if (incoming_order.type == LIMIT)
            for (resting_order& resting_order : price_levels.at(incoming_order.price))
            {
                relevant_orders.push_back(&resting_order);
                remaining_qty -= resting_order.quantity;
                if (remaining_qty < 0)
                    break;
            }
        if (incoming_order.type == MARKET)
        {
            for (std::deque<resting_order>& price_level : price_levels | std::views::values)
                for (resting_order& resting_order : price_level)
                {
                    relevant_orders.push_back(&resting_order);
                    remaining_qty -= resting_order.quantity;
                    if (remaining_qty == 0)
                        break;
                }
        }

        has_sufficient_quantity = remaining_qty < 0;
        return relevant_orders;
    }

    /// Find minimum number of resting buy/sell orders whose total quantity is
    ///  less than or equal to that of the incoming order.
    ///
    /// @param incoming_order incoming buy/sell order
    /// @param has_sufficient_quantity flag that signals whether the orders found have
    ///                                quantity equal to or in excess of the incoming order
    /// @return flat list of pointers to sell/buy orders
    std::deque<resting_order*> _find_orders(const incoming_order& incoming_order,
                                            bool& has_sufficient_quantity)
    {
        std::deque<resting_order*> resting_orders;

        if (incoming_order.side == BUY)
            matching_engine::_find_orders_impl(incoming_order, _sell_price_levels, resting_orders, has_sufficient_quantity);
        if (incoming_order.side == SELL)
            matching_engine::_find_orders_impl(incoming_order, _buy_price_levels, resting_orders, has_sufficient_quantity);

        return resting_orders;
    }

    template <typename PriceLevels>
    void _remove_order_impl(resting_order& order,
                            PriceLevels& price_levels,
                            std::deque<uint32_t>& empty_list)
    {
        if (!price_levels.contains(order.price))
            return;

        std::deque<resting_order>& orders = price_levels.at(order.price);
        const auto it = std::ranges::find(orders, order);
        if (it == orders.end()) return;

        const price_type price = order.price;
        orders.erase(it);
        if (orders.empty())
            empty_list.push_back(price);
    }
    void _remove_order(resting_order& order)
    {
        if (order.side == BUY)
            matching_engine::_remove_order_impl(order, _buy_price_levels, _empty_buy_order_price_levels);
        else
            matching_engine::_remove_order_impl(order, _sell_price_levels, _empty_sell_order_price_levels);
    }

    template <typename PriceLevels>
    bool _cancel_impl(const id_type& order_id, PriceLevels& price_levels)
    {
        for (std::deque<resting_order>& resting_order_list : price_levels | std::views::values)
            for (resting_order& resting_order : resting_order_list)
                if (resting_order.id == order_id)
                {
                    matching_engine::_remove_order(resting_order);
                    return true;
                }

        return false;
    }

    void _clean_up()
    {
        for (const auto& price : _empty_buy_order_price_levels)
            _buy_price_levels.erase(price);
        for (const auto& price : _empty_sell_order_price_levels)
            _sell_price_levels.erase(price);

        _empty_buy_order_price_levels.clear();
        _empty_sell_order_price_levels.clear();
    }

private:
    static uint32_t _new_id(uint32_t& type_id) { return type_id++; }

private:
    const std::string _stock_name;

    std::deque<uint32_t> _empty_buy_order_price_levels;
    std::deque<uint32_t> _empty_sell_order_price_levels;

    uint32_t _completed_trade_id = 0;
    std::deque<trade> _trades;
    std::map<uint32_t, std::deque<resting_order>, std::greater<>> _buy_price_levels;
    std::map<uint32_t, std::deque<resting_order>, std::less<>>  _sell_price_levels;

    matching_config _config;
};
