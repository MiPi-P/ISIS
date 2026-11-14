#include <chrono>
#include <cstdio>

void print_time(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto us = std::chrono::duration<double, std::micro>(elapsed).count();
    std::printf("Round: %.1f us\n", us);
    std::fflush(stdout);
}
