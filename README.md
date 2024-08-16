# Pinnacium

It is a microbenchmarking framework for C++ aimed at providing precise performance measurements and analysis. It integrates hardware performance counters for detailed metrics and supports CSV exporting for comprehensive result analysis.

## Features

- **High-Resolution Timing:** Accurate measurement of code execution time using `std::chrono`.
- **Hardware Performance Counters:** Access low-level CPU metrics (e.g., cache misses, branch mispredictions) via `rdpmc` instruction (x86 specific).
- **Multi-Threading Support:** Run benchmarks in parallel to evaluate performance under concurrent conditions.
- **Custom Setup/Teardown:** Define setup and teardown functions to prepare and clean up before and after each benchmark iteration.
- **CSV Exporting:** Export benchmark results to a CSV file for easy analysis and visualization.

### Prerequisites

- A C++17 or later compiler.
- For hardware performance counters, x86 architecture with appropriate permissions.

### Installation

Clone the repository:

```bash
git clone https://github.com/yourusername/OptimusBenchmark.git
cd OptimusBenchmark
```

## Usage
- Define Your benchmark function:
```cpp
void Function() {
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += i;
    }
}
```

- Create and Run a benchmark:
```c
#include "benchmark.h"

int main() {
    Benchmark bench("Benchmark", Function);
    bench.enablePerformanceCounters(false); // Optional
    bench.run();

    return 0;
}
```

- Run the Program:
```
g++ -std=c++17 -o benchmark main.cpp
./benchmark
```

## Example Output
The benchmark will output results to the console and export data to a CSV file named _results.csv:
```
Benchmark: Benchmark
Iterations: 100
Mean: 12345678 ns
Stddev: 1234 ns
Min: 12300000 ns
Max: 12400000 ns
Performance Counters:
Counter Value: 987654
=========================
Results exported to results.csv
```

## Notes
- Ensure you have the required permissions to access performance counters on your system.
- Modify the benchmark functions and parameters as needed to fit your specific use cases.

## License
This project is licensed under the MIT License.