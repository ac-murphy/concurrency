#pragma once
#include "BIN.h"

class visualise
{
public:
    visualise() = default;
    ~visualise() = default;

public:
    void price_time_chart(const order_book& book)
    {
        std::vector<uint32_t> prices;
        std::vector<long long> times;
        const auto& trades = book.trades();
        for (const trade& trade : trades)
        {
            prices.push_back(trade.price);
            times.push_back(trade.timestamp);
        }

        binary_io::write_T("prices", prices);
        binary_io::write_T("times", times);
    }

private:
    std::filesystem::path _output_dir = TEST_RESOURCE_DIR;
};