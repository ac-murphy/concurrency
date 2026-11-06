#pragma once
#include <string>
#include "types.h"
#include "event_log.h"
#include "matching_engine.h"
#include "util.h"

class order_book
{
public:
    order_book() = delete;
    ~order_book()
    {
        _log.join();
    }

    order_book(const std::string& name)
    : _stock_name(name), _matching_engine(name), _output_dir(std::filesystem::path(OUTPUT_DIR)/"books"/name)
    {
        _log.set_output_path(_output_dir/"orders.bin");
        _matching_engine.set_log_output_path(_output_dir/"trades.bin");
        order_book::start_log();
    }

public:
    id_type ask(const id_type& user_id, const quantity_type& quantity, const price_type& price = 0, const order_policy& policy = GTC)
    {
        auto ask = _make_order(SELL, user_id, quantity, price, policy);

        // Only pass order to matching engine if valid.
        if (order_book::_order_valid(ask))
            _matching_engine.match(ask);

        // Save regardless of validity.
        _log.save(ask);

        return ask.id;
    }
    id_type bid(const id_type& user_id, const quantity_type& quantity, const price_type& price = 0, const order_policy& policy = GTC)
    {
        auto bid = _make_order(BUY, user_id, quantity, price, policy);

        // Only pass order to matching engine if valid.
        if (order_book::_order_valid(bid))
            _matching_engine.match(bid);

        // Save regardless of validity.
        _log.save(bid);

        return bid.id;
    }
    bool cancel(const order_side& side, const id_type& order_id)
    {
        if (order_id == invalid_id)
            return false;

        const bool success = _matching_engine.cancel(side, order_id);
        return success;
    }

    size_t resting_buy_order_count() const
    {
        size_t count = 0;
        for (const auto& orders : _matching_engine.buy_orders() | std::views::values)
            count += orders.size();

        return count;
    }
    size_t resting_sell_order_count() const
    {
        size_t count = 0;
        for (const auto& orders : _matching_engine.sell_orders() | std::views::values)
            count += orders.size();

        return count;
    }
    std::vector<resting_order> current_buy_orders(std::optional<uint32_t> price = std::nullopt) const
    {
#if TRACY_ENABLE == 1
       ZoneScopedN("query buy orders");
#endif
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
    std::vector<resting_order> current_sell_orders(std::optional<uint32_t> price = std::nullopt) const
    {
#if TRACY_ENABLE == 1
        ZoneScopedN("query sell orders");
#endif
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
        return load_trades();
        // std::vector<trade> relevant_trades;
        //
        // for (const trade& trade : _matching_engine.trades())
        //     relevant_trades.push_back(trade);
        //
        // return relevant_trades;
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
    void start_log()
    {
        _log.start();
        _matching_engine.start_log();
    }
    void flush_log(const bool& yield = false)
    {
        _log.flush(yield);
        _matching_engine.flush_log(yield);
    }

public:
    price_type best_bid() const { return _matching_engine.buy_orders().empty()  ? invalid_price :  _matching_engine.buy_orders().begin()->first; }
    price_type best_ask() const { return _matching_engine.sell_orders().empty() ? invalid_price : _matching_engine.sell_orders().begin()->first; }

public:
    const auto& sell_order_price_levels() const
    {
        return _matching_engine.sell_orders();
    }
    const auto& buy_order_price_levels() const
    {
        return _matching_engine.buy_orders();
    }
    const auto& trades() const
    {
        return _matching_engine.trades();
    }
    std::vector<trade> load_trades() const
    {
        return order_book::_load_impl<trade>("trades.bin");
    }
    std::vector<incoming_order> load_buy_orders() const
    {
        return order_book::_load_orders_impl("orders.bin", BUY);
    }
    std::vector<incoming_order> load_sell_orders() const
    {
        return order_book::_load_orders_impl("orders.bin", SELL);
    }

private:
    std::vector<incoming_order> _load_orders_impl(const std::string& filename, const order_side& side) const
    {
        std::vector<incoming_order> orders = order_book::_load_impl<incoming_order>(filename);
        std::vector<incoming_order> correct_side_orders;
        correct_side_orders.reserve(orders.size());
        for (auto&& order : orders)
            if (order.side == side)
                correct_side_orders.push_back(std::move(order));

        return correct_side_orders;
    }
    template <typename T>
    std::vector<T> _load_impl(const std::string& filename) const
    {
        const std::filesystem::path path(_output_dir/filename);
        if (!std::filesystem::exists(path))
            return {};

        const auto file_size = std::filesystem::file_size(path);
        constexpr auto obj_size = sizeof(T);
        if (file_size % obj_size != 0)
            return {};

        std::ifstream in(path, std::ios::binary | std::ios::in);
        if (!in)
            return {};

        const auto count = file_size / obj_size;
        std::vector<T> list(count);
        in.read(reinterpret_cast<char*>(list.data()), file_size);

        return list;
    }

private:
    bool _order_ok(const incoming_order& order) const
    {
#if TRACY_ENABLE == 1
        ZoneScopedN("_order_ok");
#endif
        if (order.quantity < _config.min_quantity || order.quantity > _config.max_quantity)
        {
            debug_print(std::format("order_book::_order_ok \"failed; invalid order quantity (qty: {}, min_ok: {}, max_ok: {})\"",
                                    order.quantity, _config.min_quantity, _config.max_quantity));
            return false;
        }
        if (order.price < _config.min_price       || order.price > _config.max_price)
        {
            debug_print(std::format("order_book::_order_ok \"failed; invalid order price (price: {}, min_ok: {}, max_ok: {})\"",
                                    order.price, _config.min_price, _config.max_price));
            return false;
        }
        if (order.quantity % _config.lot_size != 0)
        {
            debug_print(std::format("order_book::_order_ok \"failed; invalid order quantity (qty: {}, lot_size: {})\"",
                                    order.quantity, _config.lot_size));
            return false;
        }
        if (order.price % _config.tick_size != 0)
        {
            debug_print(std::format("order_book::_order_ok \"failed; invalid order price (qty: {}, tick_size: {})\"",
                                    order.quantity, _config.tick_size));
            return false;
        }

        return true;
    }
    bool _order_valid(const incoming_order& order) const
    {
#if TRACY_ENABLE == 1
        ZoneScopedN("_order_valid");
#endif
        if (order.id        == invalid_id)       return false;
        if (order.user_id   == invalid_id)       return false;
        if (order.quantity  == invalid_quantity) return false;
        if (order.price     == invalid_price)    return false;
        if (order.timestamp == invalid_time)     return false;

        return true;
    }
    incoming_order _make_order(const order_side& side, const id_type& user_id, const quantity_type& quantity, const price_type& price, const order_policy& policy)
    {
#if TRACY_ENABLE == 1
        ZoneScopedN("_make_order");
#endif
        incoming_order order;
        order.type = price == 0 ? MARKET : LIMIT;
        order.side = side;
        order.policy = policy;
        order.quantity = quantity;
        order.price = price;
        order.id = _new_id(_order_id);
        order.user_id = user_id;
        order.timestamp = order_book::_new_timestamp();

        if (!order_book::_order_ok(order))
        {
#if TRACY_ENABLE == 1
            ZoneScopedN("return invalid order");
#endif
            return invalid_order;
        }
        else
            return order;
    }

private:
    static id_type _new_id(id_type& type_id) { return type_id++; }
    static time_type _new_timestamp()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

private:
    uint32_t _sell_order_id = 0;
    uint32_t _buy_order_id = 0;
    id_type _order_id = 0;
    std::filesystem::path _output_dir;
    event_log<incoming_order, 1'000, 100> _log;

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
        return book.bid(_id, shares, price, policy);
    }
    id_type limit_ask(order_book& book, const uint32_t& shares, const uint32_t& price, const order_policy& policy = GTC) const
    {
        return book.ask(_id, shares, price, policy);
    }
    id_type market_bid(order_book& book, const uint32_t& shares, const order_policy& policy = GTC) const
    {
        return book.bid(_id, shares, 0, policy);
    }
    id_type market_ask(order_book& book, const uint32_t& shares, const order_policy& policy = GTC) const
    {
        return book.ask(_id, shares, 0, policy);
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

template <typename T>
std::vector<T> load(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return {};

    const auto file_size = std::filesystem::file_size(path);
    constexpr auto obj_size = sizeof(T);
    if (file_size % obj_size != 0)
        return {};

    std::ifstream in(path, std::ios::binary | std::ios::in);
    if (!in)
        return {};

    const auto count = file_size / obj_size;
    std::vector<T> list(count);
    in.read(reinterpret_cast<char*>(list.data()), file_size);

    return list;
}