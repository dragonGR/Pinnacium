# Pinnacium

Pinnacium is a lightweight C++ microbenchmarking library for measuring small pieces of code with repeated runs, warmup passes, optional setup and teardown hooks, and an optional x86 `rdpmc` path for low-level counter sampling.

This repository appears to be an early single-library experiment rather than a fully productized benchmarking suite. The current codebase exposes two runners:

- `Benchmark` for single-threaded measurements.
- `MultiThreadedBenchmark` for running the same workload concurrently across a fixed number of worker threads.

The project now reports results to standard output only. CSV export has been removed so the library stays focused on measurement rather than result persistence.

## Current Status

The repository had only one historical commit and minimal documentation, so this README is based on the implementation that is present today in [benchmark.h](benchmark.h) and [benchmark.cpp](benchmark.cpp).

Today, Pinnacium is best understood as:

- A small embeddable benchmarking helper for local experiments.
- A starting point for latency-oriented measurements in nanoseconds.
- An x86-first utility with optional direct performance-counter sampling.

It is not yet:

- A statistically rigorous replacement for Google Benchmark or Criterion.
- A reporting pipeline with file export, charting, or historical storage.
- A cross-platform abstraction over hardware counters.

## What The Library Does

### `Benchmark`

`Benchmark` runs a callable repeatedly on the current thread and records one duration sample per measured iteration.

It supports:

- Warmup iterations to reduce one-time startup noise.
- Optional setup and teardown callables around each run.
- Optional x86/x86_64 performance-counter reads through `rdpmc`.
- Console summaries for mean, standard deviation, minimum, and maximum latency.

### `MultiThreadedBenchmark`

`MultiThreadedBenchmark` launches a fixed-size thread group for each measured iteration and records one duration sample per worker execution.

That means:

- `iterations = 100` and `threads = 8` produce up to `800` latency samples.
- Reported statistics are aggregated across all collected thread samples.
- This is useful for rough concurrency experiments, but it does not model coordinated start barriers, affinity pinning, or scheduler isolation.

## Language And Compiler Support

Pinnacium now targets **C++23**, which is the latest published C++ language standard. The build configuration in [CMakeLists.txt](CMakeLists.txt) requires `cxx_std_23`.

Practical compiler guidance:

- Use a current GCC, Clang, or MSVC release with strong C++23 support.
- On this machine, the library was checked with `g++ 15.2.1` and `clang++ 22.1.0`.
- For broader compiler feature tracking, see cppreference's compiler support tables: https://en.cppreference.com/w/cpp/compiler_support/

## Platform Notes

The timing path is portable C++, but the performance-counter path is not.

- `enablePerformanceCounters(true)` is only meaningful on `x86` and `x86_64`.
- The library uses the `rdpmc` instruction directly.
- Even on supported CPUs, counter access depends on OS and kernel configuration.
- If performance counters are requested on an unsupported build target, Pinnacium leaves them disabled and prints a warning to `stderr`.

## Repository Layout

- [benchmark.h](benchmark.h): public API declarations.
- [benchmark.cpp](benchmark.cpp): benchmark runner implementation.
- [examples/basic_benchmark.cpp](examples/basic_benchmark.cpp): small end-to-end usage example.
- [CMakeLists.txt](CMakeLists.txt): modern CMake build entrypoint targeting C++23.

## Building

### With CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces:

- `pinnacium`: the library target.
- `pinnacium_basic`: a small example executable.

### With A Direct Compiler Invocation

```bash
g++ -std=c++23 -O3 -pthread benchmark.cpp examples/basic_benchmark.cpp -I. -o pinnacium_basic
```

## Usage Example

```cpp
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
}
```

### Setup And Teardown Hooks

```cpp
Benchmark benchmark("prepare_and_run", [] {
    // workload
});

benchmark.setSetupFunction([] {
    // per-iteration preparation
});

benchmark.setTeardownFunction([] {
    // per-iteration cleanup
});

benchmark.run();
```

### Multi-Threaded Run

```cpp
MultiThreadedBenchmark benchmark(
    "parallel_work",
    [] {
        volatile int value = 0;
        for (int i = 0; i < 10'000; ++i) {
            value += i;
        }
    },
    100,
    10,
    8);

benchmark.run();
```

## Example Output

```text
Benchmark: sum_vector
Iterations: 250
Samples: 250
Mean: 3168 ns
Stddev: 147 ns
Min: 3014 ns
Max: 3671 ns
=========================
```

For `MultiThreadedBenchmark`, the output additionally includes the configured thread count and the total number of collected samples.

## Design Trade-Offs

This code intentionally stays compact, but that simplicity comes with trade-offs:

- Timing uses wall-clock measurement around the full callable body.
- There is no dead-code-elimination guard beyond what the benchmarked code itself does.
- There is no percentile reporting, no outlier filtering, and no confidence interval estimation.
- Multi-threaded runs do not synchronize worker start times beyond normal thread launch behavior.

If you need publication-quality benchmarking or advanced statistical analysis, you should likely migrate the workload to a more mature framework.

## Recent Cleanup In This Refresh

The project has been refreshed to make the repository easier to understand and reuse:

- Removed CSV export from the public behavior and implementation.
- Raised the baseline language target to C++23.
- Added a CMake build file and a runnable example.
- Tightened the implementation for repeated runs and safer multi-threaded sample collection.
- Rewrote the documentation around the actual current code instead of the stale historical description.

## License

No standalone license file is currently checked into this repository. If you plan to publish or distribute Pinnacium, add an explicit license before doing so.
