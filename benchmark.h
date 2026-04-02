#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

// Benchmark measures a callable repeatedly and reports latency statistics.
// It is intended for quick microbenchmark experiments where a lightweight
// single-header-style API is more important than framework integration.
class Benchmark {
public:
    using BenchmarkFunction = std::function<void()>;

    Benchmark(std::string name, BenchmarkFunction fn, int iterations = 100, int warmup = 10);

    void run();
    void setSetupFunction(BenchmarkFunction setup);
    void setTeardownFunction(BenchmarkFunction teardown);
    void enablePerformanceCounters(bool enable);

private:
    void resetMeasurements();
    void warmUp();
    void measure();
    void printResults();

    void startPerfCounters();
    void stopPerfCounters();
    static bool supportsPerformanceCounters();

    std::string name_;
    BenchmarkFunction function_;
    BenchmarkFunction setupFunction_;
    BenchmarkFunction teardownFunction_;
    int iterations_;
    int warmup_;
    bool usePerformanceCounters_ = false;
    std::vector<long long> results_;
    std::vector<uint64_t> performanceCounters_;
};

// MultiThreadedBenchmark executes the benchmark body concurrently across a
// fixed number of worker threads and aggregates all observed samples.
class MultiThreadedBenchmark {
public:
    MultiThreadedBenchmark(std::string name, Benchmark::BenchmarkFunction fn, int iterations = 100, int warmup = 10, int threads = std::thread::hardware_concurrency());

    void run();
    void setSetupFunction(Benchmark::BenchmarkFunction setup);
    void setTeardownFunction(Benchmark::BenchmarkFunction teardown);
    void enablePerformanceCounters(bool enable);

private:
    void resetMeasurements();
    void warmUp();
    void measure();
    void runInThreads(const std::function<void()>& task);
    void printResults();

    void startPerfCounters();
    void stopPerfCounters();
    static bool supportsPerformanceCounters();

    std::string name_;
    Benchmark::BenchmarkFunction function_;
    Benchmark::BenchmarkFunction setupFunction_;
    Benchmark::BenchmarkFunction teardownFunction_;
    int iterations_;
    int warmup_;
    int threads_;
    bool usePerformanceCounters_ = false;
    std::vector<long long> results_;
    std::vector<uint64_t> performanceCounters_;
    std::mutex mutex_;
};

#endif // BENCHMARK_H
