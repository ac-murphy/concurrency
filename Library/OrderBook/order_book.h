#pragma once
#include <string>
#include "types.h"
#include "matching_engine.h"

class order_book
{
public:
    order_book() = delete;
    ~order_book() = default;

    order_book(const std::string& name)
    : _stock_name(name), _matching_engine(name)
    {
        stock stock(name);
        _matching_engine.log_state();
    }

public:
    uint32_t ask(const uint32_t& id, const uint32_t shares, const uint32_t& price = 0)
    {
        if (price > 0)
        {
            limit_order<SELL> ask;
            ask.id = new_id(_sell_order_id);
            ask.user_id = id;
            ask.stock_name = _stock_name;
            ask.price = price;
            ask.shares = shares;

            _matching_engine.match(ask);
            _matching_engine.log_state();
            return ask.id;
        }
        else
        {
            market_order<SELL> ask;
            ask.id = new_id(_sell_order_id);
            ask.user_id = id;
            ask.stock_name = _stock_name;
            ask.shares = shares;

            _matching_engine.match(ask);
            _matching_engine.log_state();
            return ask.id;
        }
    }
    uint32_t bid(const uint32_t& id, const uint32_t shares, const uint32_t& price = 0)
    {
        if (price > 0)
        {
            limit_order<BUY> bid;
            bid.id = new_id(_buy_order_id);
            bid.user_id = id;
            bid.stock_name = _stock_name;
            bid.price = price;
            bid.shares = shares;

            _matching_engine.match(bid);
            _matching_engine.log_state();
            return bid.id;
        }
        else
        {
            market_order<BUY> bid;
            bid.id = new_id(_buy_order_id);
            bid.user_id = id;
            bid.stock_name = _stock_name;
            bid.shares = shares;

            _matching_engine.match(bid);
            _matching_engine.log_state();
            return bid.id;
        }
    }

    template <order_side T>
    std::vector<limit_order<T>> query_resting_orders(std::optional<std::string> stock_name = std::nullopt,
                                                     std::optional<uint32_t> price = std::nullopt,
                                                     std::optional<uint32_t> user_id = std::nullopt) const
    {
        std::vector<limit_order<T>> relevant_orders;
        const auto index = [&]
        {
            if constexpr      (T == BUY)  { return _matching_engine.buy_orders(); }
            else if constexpr (T == SELL) { return _matching_engine.sell_orders(); }
            else { throw std::logic_error("unknown order type"); }
        }();

        for (const auto& [price_, orders] : index)
        {
            if (price.has_value() && price_ != price) continue;

            for (const auto& order : orders)
            {
                if (user_id.has_value() && order.user_id != user_id) continue;

                relevant_orders.push_back(order);
            }
        }

        return relevant_orders;
    }
    std::vector<trade> query_trades(std::optional<uint32_t> merchant_id = std::nullopt,
                                    std::optional<uint32_t> recipient_id = std::nullopt) const
    {
        std::vector<trade> relevant_trades;

        for (const trade& trade : _matching_engine.trades())
        {
            if (merchant_id.has_value() && trade.merchant_id != merchant_id) continue;
            if (recipient_id.has_value() && trade.recipient_id != recipient_id) continue;
            relevant_trades.push_back(trade);
        }

        return relevant_trades;
    }

public:
    const auto& sell_orders() const { return _matching_engine.sell_orders(); }
    const auto& buy_orders() const { return _matching_engine.buy_orders(); }
    const auto& trades() const { return _matching_engine.trades(); }

private:
    static uint32_t new_id(uint32_t& type_id) { return type_id++; }

private:
    uint32_t _sell_order_id = 0;
    uint32_t _buy_order_id = 0;

    std::string _stock_name;
    matching_engine _matching_engine;
};

class client
{
public:
    explicit client() : _id(_client_id++) {}
    ~client() = default;

public:
    void ask(order_book& book, const uint32_t shares, const uint32_t& price = 0) const
    {
        book.ask(_id, shares, price);
    }
    void bid(order_book& book, const uint32_t shares, const uint32_t& price = 0) const
    {
        book.bid(_id, shares, price);
    }

public:
    int32_t id() const { return _id; }

private:
    inline static uint32_t _client_id = 0;

private:
    uint32_t _id;
};