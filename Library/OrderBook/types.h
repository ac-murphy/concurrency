#pragma once
#include <format>
#include <string>
#include "nlohmann/json.hpp"

using id_type = uint32_t;
using price_type = uint32_t;
using quantity_type = uint32_t;
using time_type = uint32_t;

enum order_type
{
    LIMIT,
    MARKET
};
enum order_side
{
    BUY,
    SELL
};
enum order_policy
{
    GTC,
    FOK,
    IOC
};

template <order_type T, order_side S, order_policy P>
struct incoming_order
{
    static constexpr order_type type = T;
    static constexpr order_side side = S;
    static constexpr order_policy policy = P;

    id_type id;
    price_type price;
    quantity_type quantity;
};
template <order_side S>
struct resting_order
{
    static constexpr order_side side = S;

    id_type id;
    price_type price;
    quantity_type quantity;
    time_type good_until;
};

// todo: make limit_order and market_order separate types to save memory
//  as market_order doesn't specify a price.
template <order_side S, order_policy P = GTC>
using limit_order = incoming_order<LIMIT, S, P>;
template <order_side S, order_policy P = GTC>
using market_order = incoming_order<MARKET, S, P>;

template <typename T>                                 struct order_like_trait                          : std::false_type {};
template <order_type T, order_side S, order_policy P> struct order_like_trait<incoming_order<T, S, P>> : std::true_type  {};
template <order_side S>                               struct order_like_trait<resting_order<S>>        : std::true_type  {};
template <typename T> concept order_like = order_like_trait<T>::value;

template <typename T>                                 struct incoming_order_like_trait                          : std::false_type {};
template <order_type T, order_side S, order_policy P> struct incoming_order_like_trait<incoming_order<T, S, P>> : std::true_type  {};
template <typename T> concept incoming_order_like = incoming_order_like_trait<T>::value;


template <typename T>   struct resting_order_like_trait                   : std::false_type {};
template <order_side S> struct resting_order_like_trait<resting_order<S>> : std::true_type  {};
template <typename T> concept resting_order_like = resting_order_like_trait<T>::value;

template <typename T> concept limit_order_like         = order_like<T> && T::type == LIMIT;
template <typename T> concept market_order_like        = order_like<T> && T::type == MARKET;
template <typename T> concept restable_order           = order_like<T> && T::policy == GTC;
template <typename T> concept partially_fillable_order = order_like<T> && T::policy != FOK;

struct trade
{
    id_type id;
    id_type ask_id;
    id_type bid_id;
    price_type price;
    quantity_type quantity;
    long long timestamp;
};

template <order_side T>
bool operator==(const limit_order<T>& a, const limit_order<T>& b)
{
    return a.id == b.id;
}

template <order_like T>
bool operator==(const T& a, const T& b)
{
    return a.id == b.id;
}

inline std::string price_to_string(const uint32_t& price)
{
    const auto& pounds = price / 100;
    const auto& pence = price % 100;

    return std::format("£{}.{:02}", pounds, pence);
}
inline uint32_t price_from_string(const std::string& price)
{
    const auto separator_position = price.find('.');
    const int32_t pounds = std::stoi(price.substr(1, separator_position));
    const int32_t pence = std::stoi(price.substr(separator_position + 1));

    return (pounds * 100) + pence;
}

inline void to_json(nlohmann::json& j, const limit_order<BUY>& buy_order)
{
    j["id"] = buy_order.id;
    j["price"] = price_to_string(buy_order.price);
    j["shares"] = buy_order.quantity;
}
inline void from_json(const nlohmann::json& j, limit_order<BUY>& buy_order)
{
    j.at("id").get_to(buy_order.id);
    buy_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(buy_order.quantity);
}
inline void to_json(nlohmann::json& j, const limit_order<SELL>& sell_order)
{
    j["id"] = sell_order.id;
    j["price"] = price_to_string(sell_order.price);
    j["shares"] = sell_order.quantity;
}
inline void from_json(const nlohmann::json& j, limit_order<SELL>& sell_order)
{
    j.at("id").get_to(sell_order.id);
    sell_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(sell_order.quantity);
}
template <resting_order_like T> void to_json(nlohmann::json& j, const T& sell_order)
{
    j["id"] = sell_order.id;
    j["price"] = price_to_string(sell_order.price);
    j["shares"] = sell_order.quantity;
}
template <resting_order_like T> void from_json(const nlohmann::json& j, T& sell_order)
{
    j.at("id").get_to(sell_order.id);
    sell_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(sell_order.quantity);
}
inline void to_json(nlohmann::json& j, const trade& completed_order)
{
    j["id"] = completed_order.id;
    j["ask_id"] = completed_order.ask_id;
    j["bid_id"] = completed_order.bid_id;
    j["price"] = price_to_string(completed_order.price);
    j["shares"] = completed_order.quantity;
}
inline void from_json(const nlohmann::json& j, trade& completed_order)
{
    j.at("id").get_to(completed_order.id);
    j.at("ask_id").get_to(completed_order.ask_id);
    j.at("bid_id").get_to(completed_order.bid_id);
    completed_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(completed_order.quantity);
}

inline std::ostream& operator<<(std::ostream& os, const limit_order<BUY>& bid)
{
    os << "bid:" << std::endl
       << "\tid: " << bid.id << std::endl
       << "\tprice: " << bid.price << std::endl
       << "\tshares: " << bid.quantity << std::endl;

    return os;
}
inline std::ostream& operator<<(std::ostream& os, const limit_order<SELL>& ask)
{
    os << "ask:" << std::endl
       << "\tid: " << ask.id << std::endl
       << "\tprice: " << ask.price << std::endl
       << "\tshares: " << ask.quantity << std::endl;

    return os;
}