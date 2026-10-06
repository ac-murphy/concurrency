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
        _matching_engine.log_state();
    }

public:
    id_type ask(const quantity_type& quantity, const price_type& price = 0, const order_policy& policy = GTC)
    {
        if (!order_book::order_ok(quantity, price, policy))
            return invalid_id;

        incoming_order ask;
        ask.type = price == 0 ? MARKET : LIMIT;
        ask.side = SELL;
        ask.policy = policy;
        ask.quantity = quantity;
        ask.price = price;
        ask.id = new_id(_sell_order_id);

        _matching_engine.match(ask);
        _matching_engine.log_state();
        return ask.id;
    }
    id_type bid(const quantity_type& quantity, const price_type& price = 0, const order_policy& policy = GTC)
    {
        if (!order_book::order_ok(quantity, price, policy))
            return invalid_id;

        incoming_order bid;
        bid.type = price == 0 ? MARKET : LIMIT;
        bid.side = BUY;
        bid.policy = policy;
        bid.quantity = quantity;
        bid.price = price;
        bid.id = new_id(_buy_order_id);

        _matching_engine.match(bid);
        _matching_engine.log_state();
        return bid.id;
    }
    bool cancel(const order_side& side, const id_type& order_id)
    {
        if (order_id == invalid_id)
            return false;

        bool success = _matching_engine.cancel(side, order_id);
        _matching_engine.log_state();
        return success;
    }

    std::vector<resting_order> query_resting_buy_orders(std::optional<uint32_t> price = std::nullopt) const
    {
        std::vector<resting_order> relevant;
        for (const auto& [price_, orders] : _matching_engine.buy_orders())
        {
            if (price.has_value() && price_ != price) continue;

            for (const auto& order : orders)
            {
                relevant.push_back(order);
            }
        }

        return relevant;
    }
    std::vector<resting_order> query_resting_sell_orders(std::optional<uint32_t> price = std::nullopt) const
    {
        std::vector<resting_order> relevant;
        for (const auto& [price_, orders] : _matching_engine.sell_orders())
        {
            if (price.has_value() && price_ != price) continue;

            for (const auto& order : orders)
            {
                relevant.push_back(order);
            }
        }

        return relevant;
    }
    std::vector<trade> query_trades(std::optional<uint32_t> merchant_id = std::nullopt,
                                    std::optional<uint32_t> recipient_id = std::nullopt) const
    {
        std::vector<trade> relevant_trades;

        for (const trade& trade : _matching_engine.trades())
        {
            // if (merchant_id.has_value() && trade.merchant_id != merchant_id) continue;
            // if (recipient_id.has_value() && trade.recipient_id != recipient_id) continue;
            relevant_trades.push_back(trade);
        }

        return relevant_trades;
    }

public:
    void set_market_config(const market_config& config)
    {
        _config = config;
    }
    void set_matching_config(const matching_config& config)
    {
        _matching_engine.set_config(config);
    }

public:
    uint32_t best_bid() const
    {
        return _matching_engine.buy_orders().begin()->first;
    }
    uint32_t best_ask() const { return _matching_engine.sell_orders().begin()->first; }

public:
    const auto& sell_orders() const { return _matching_engine.sell_orders(); }
    const auto& buy_orders() const { return _matching_engine.buy_orders(); }
    const auto& trades() const { return _matching_engine.trades(); }

private:
    bool order_ok(const quantity_type& quantity, const price_type& price, const order_policy& policy)
    {
        if (quantity < _config.min_quantity || quantity > _config.max_quantity) return false;
        if (price < _config.min_price       || price > _config.max_price)       return false;

        return true;
    }

private:
    static uint32_t new_id(uint32_t& type_id) { return type_id++; }

private:
    uint32_t _sell_order_id = 0;
    uint32_t _buy_order_id = 0;

    std::string _stock_name;
    market_config _config;
    matching_engine _matching_engine;
};

class client
{
public:
    explicit client() : _id(_client_id++) {}
    ~client() = default;

public:
    id_type limit_bid(order_book& book, const uint32_t& shares, const uint32_t& price, const order_policy& policy = GTC) const
    {
        return book.bid(shares, price, policy);
    }
    id_type limit_ask(order_book& book, const uint32_t& shares, const uint32_t& price, const order_policy& policy = GTC) const
    {
        return book.ask(shares, price, policy);
    }
    id_type market_bid(order_book& book, const uint32_t& shares, const order_policy& policy = GTC) const
    {
        return book.bid(shares, 0, policy);
    }
    id_type market_ask(order_book& book, const uint32_t& shares, const order_policy& policy = GTC) const
    {
        return book.ask(shares, 0, policy);
    }
    bool cancel_bid(order_book& book, const id_type& id)
    {
        return book.cancel(order_side::BUY, id);
    }
    bool cancel_ask(order_book& book, const id_type& id)
    {
        return book.cancel(order_side::SELL, id);
    }

public:
    int32_t id() const { return _id; }

private:
    inline static uint32_t _client_id = 0;

private:
    uint32_t _id;
};