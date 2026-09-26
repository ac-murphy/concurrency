#pragma once
#include <format>
#include <string>
#include "nlohmann/json.hpp"

namespace policy
{
    namespace time_in_force
    {
        struct GTC {};
    }
}

struct stock
{
    std::string name;
};

struct user
{
    uint32_t id;
};

enum order_side
{
    BUY,
    SELL,
};

template <order_side T>
struct limit_order
{
    uint32_t id;
    uint32_t user_id;
    static constexpr order_side side = T;

    uint32_t price;
    uint32_t shares;
    std::string stock_name;
};

template <order_side T>
struct market_order
{
    uint32_t id;
    uint32_t user_id;
    static constexpr order_side side = T;

    uint32_t shares;
    std::string stock_name;
};

template <typename T>
concept order_like = std::same_as<T, limit_order<T::side>> || std::same_as<T, market_order<T::side>>;

struct trade
{
    uint32_t id;
    uint32_t merchant_id;
    uint32_t recipient_id;

    uint32_t price;
    uint32_t shares;
    std::string stock_name;
};

template <order_side T>
bool operator==(const limit_order<T>& a, const limit_order<T>& b)
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
    j["user_id"] = buy_order.user_id;
    j["price"] = price_to_string(buy_order.price);
    j["shares"] = buy_order.shares;
    j["stock_name"] = buy_order.stock_name;
}
inline void from_json(const nlohmann::json& j, limit_order<BUY>& buy_order)
{
    j.at("id").get_to(buy_order.id);
    j.at("user_id").get_to(buy_order.user_id);
    // j.at("price").get_to(buy_order.price);
    buy_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(buy_order.shares);
    j.at("stock_name").get_to(buy_order.stock_name);
}
inline void to_json(nlohmann::json& j, const limit_order<SELL>& sell_order)
{
    j["id"] = sell_order.id;
    j["user_id"] = sell_order.user_id;
    j["price"] = price_to_string(sell_order.price);
    j["shares"] = sell_order.shares;
    j["stock_name"] = sell_order.stock_name;
}
inline void from_json(const nlohmann::json& j, limit_order<SELL>& sell_order)
{
    j.at("id").get_to(sell_order.id);
    j.at("user_id").get_to(sell_order.user_id);
    sell_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(sell_order.shares);
    j.at("stock_name").get_to(sell_order.stock_name);
}
inline void to_json(nlohmann::json& j, const trade& completed_order)
{
    j["id"] = completed_order.id;
    j["merchant_id"] = completed_order.merchant_id;
    j["recipient_id"] = completed_order.recipient_id;
    j["price"] = price_to_string(completed_order.price);
    j["shares"] = completed_order.shares;
    j["stock_name"] = completed_order.stock_name;
}
inline void from_json(const nlohmann::json& j, trade& completed_order)
{
    j.at("id").get_to(completed_order.id);
    j.at("merchant_id").get_to(completed_order.merchant_id);
    j.at("recipient_id").get_to(completed_order.recipient_id);
    completed_order.price = price_from_string(j.at("price"));
    j.at("shares").get_to(completed_order.shares);
    j.at("stock_name").get_to(completed_order.stock_name);
}

inline std::ostream& operator<<(std::ostream& os, const limit_order<BUY>& bid)
{
    os << "bid:" << std::endl
       << "\tid: " << bid.id << std::endl
       << "\tparticipant_id: " << bid.user_id << std::endl
       << "\tstock_name: " << bid.stock_name << std::endl
       << "\tprice: " << bid.price << std::endl
       << "\tshares: " << bid.shares << std::endl;

    return os;
}
inline std::ostream& operator<<(std::ostream& os, const limit_order<SELL>& ask)
{
    os << "ask:" << std::endl
       << "\tid: " << ask.id << std::endl
       << "\tparticipant_id: " << ask.user_id << std::endl
       << "\tstock_name: " << ask.stock_name << std::endl
       << "\tprice: " << ask.price << std::endl
       << "\tshares: " << ask.shares << std::endl;

    return os;
}