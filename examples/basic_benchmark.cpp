#include "benchmark.h"

#include <vector>

int main() {
    std::vector<int> values(10'000, 7);

    Benchmark benchmark(
        "sum_vector",
        [&values] {
            volatile long long sum = 0;
            for (int value : values) {
                sum += value;
            }
        },
        250,
        25);

    benchmark.run();
    return 0;
}
