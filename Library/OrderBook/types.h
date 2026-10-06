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

struct incoming_order
{
    order_type type;
    order_side side;
    order_policy policy;

    quantity_type quantity;
    price_type price;
    id_type id;
};
struct resting_order
{
    order_side side;

    quantity_type quantity;
    price_type price;
    id_type id;
    time_type good_until;
};
struct trade
{
    id_type id;
    id_type ask_id;
    id_type bid_id;
    price_type price;
    quantity_type quantity;
    long long timestamp;
};
using price_level = std::unordered_map<id_type, resting_order>;

template <typename T>
concept order_like = std::same_as<T, resting_order> || std::same_as<T, incoming_order>;

template <order_like T, order_like U>
bool operator==(const T& a, const U& b)
{
    return a.id == b.id;
}

// template <order_like T>
// constexpr bool is_resting_order(const T& order)
// {
//     return std::same_as<T, resting_order>;
// }
template <typename T> concept is_resting_order = std::same_as<T, resting_order>;
inline bool is_restable_order(const incoming_order& order)
{
    return order.policy == order_policy::GTC;
}
template <order_like T>
bool is_fixed_price_order(const T& order)
{
    if constexpr (is_resting_order<T>)
        return true;
    else
        return order.type == LIMIT;
}
template <order_like T>
bool is_variable_price_order(const T& order)
{
    return !is_fixed_price_order(order);
}
inline bool is_partially_fillable_order(const incoming_order& order)
{
    return order.policy != FOK;
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
inline void to_json(nlohmann::json& j, const incoming_order& incoming_order)
{
    j["id"] = incoming_order.id;
    j["price"] = price_to_string(incoming_order.price);
    j["shares"] = incoming_order.quantity;
}
inline void from_json(const nlohmann::json& j, incoming_order& incoming_order)
{
    j.at("id").get_to(incoming_order.id);
    incoming_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(incoming_order.quantity);
}
inline void to_json(nlohmann::json& j, const resting_order& resting_order)
{
    j["id"] = resting_order.id;
    j["price"] = price_to_string(resting_order.price);
    j["shares"] = resting_order.quantity;
}
inline void from_json(const nlohmann::json& j, resting_order& resting_order)
{
    j.at("id").get_to(resting_order.id);
    resting_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(resting_order.quantity);
}
inline void to_json(nlohmann::json& j, const trade& trade)
{
    j["id"] = trade.id;
    j["ask_id"] = trade.ask_id;
    j["bid_id"] = trade.bid_id;
    j["price"] = price_to_string(trade.price);
    j["shares"] = trade.quantity;
}
inline void from_json(const nlohmann::json& j, trade& trade)
{
    j.at("id").get_to(trade.id);
    j.at("ask_id").get_to(trade.ask_id);
    j.at("bid_id").get_to(trade.bid_id);
    trade.price = price_from_string(j.at("price"));
    j.at("shares").get_to(trade.quantity);
}


// inline std::ostream& operator<<(std::ostream& os, const resting_order& resting_order)
// {
//     os << "bid:" << std::endl
//        << "\tid: " << resting_order.id << std::endl
//        << "\tprice: " << resting_order.price << std::endl
//        << "\tshares: " << resting_order.quantity << std::endl;
//
//     return os;
// }
// inline std::ostream& operator<<(std::ostream& os, const limit_order<SELL>& ask)
// {
//     os << "ask:" << std::endl
//        << "\tid: " << ask.id << std::endl
//        << "\tprice: " << ask.price << std::endl
//        << "\tshares: " << ask.quantity << std::endl;
//
//     return os;
// }