### Benchmarks
#### Constant volatility and interest rate
10/10/2026, initial benchmark.
1. `closed_form_solver` `31.2ms`
2. `explicit_scheme_solver` `14.4ms`
3. `implicit_scheme_solver` `>750ms`

Difference between (1) and (2) likely due to repeated use of `std::erf` in `closed_form_solver`. The `implicit_scheme_solver` is solving a system of equations at each step, and difference in performance is striking.

