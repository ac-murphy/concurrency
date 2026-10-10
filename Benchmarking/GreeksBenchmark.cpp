#include <random>

#include "benchmark/benchmark.h"
#include "Greeks/BlackScholes.h"

template <typename T>
requires (std::is_arithmetic_v<T>)
T random(T min, T max)
{
    thread_local std::default_random_engine generator;
    std::uniform_real_distribution<T> distribution(min, max);
    return distribution(generator);
}

using solution_fn = black_scholes::solution (*)(const black_scholes::input&, const black_scholes::config&);

template <solution_fn func>
static void BM_BlackScholes(benchmark::State& state)
{
    black_scholes::input input;
    input.strike_price = random<float>(0.9f, 1.1f);
    input.duration = 1.0f;
    input.volatility = random<float>(0.1f, 0.4f);
    input.interest_rate = random<float>(0.01, 0.05f);

    black_scholes::config config;
    config.maxmium_stock_price = 1.5f;
    config.stock_price_step = 0.01f;
    config.time_step = 0.0001f;

    for (auto _ : state)
    {
        black_scholes::solution solution = func(input, config);
        benchmark::DoNotOptimize(solution);
    }
}

BENCHMARK(BM_BlackScholes<&black_scholes::closed_form_solver>);
BENCHMARK(BM_BlackScholes<&black_scholes::explicit_scheme_solver>);
BENCHMARK(BM_BlackScholes<&black_scholes::implicit_scheme_solver>);
BENCHMARK_MAIN();