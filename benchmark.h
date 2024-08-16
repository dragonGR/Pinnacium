#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <numeric>
#include <thread>
#include <mutex>
#include <fstream>
#include <cmath>
#include <immintrin.h> // For RDPMC (x86-specific)

class Benchmark {
public:
    using BenchmarkFunction = std::function<void()>;

    Benchmark(std::string name, BenchmarkFunction fn, int iterations = 100, int warmup = 10);

    void run();
    void setSetupFunction(BenchmarkFunction setup);
    void setTeardownFunction(BenchmarkFunction teardown);
    void enablePerformanceCounters(bool enable);

private:
    void warmUp();
    void measure();
    void printResults();
    void exportResults();

    void startPerfCounters();
    void stopPerfCounters();

    std::string name_;
    BenchmarkFunction function_;
    BenchmarkFunction setupFunction_;
    BenchmarkFunction teardownFunction_;
    int iterations_;
    int warmup_;
    bool usePerformanceCounters_ = false;
    uint64_t prevCounter_ = 0;
    std::vector<long long> results_;
    std::vector<uint64_t> performanceCounters_;
};

class MultiThreadedBenchmark {
public:
    MultiThreadedBenchmark(std::string name, Benchmark::BenchmarkFunction fn, int iterations = 100, int warmup = 10, int threads = std::thread::hardware_concurrency());

    void run();
    void setSetupFunction(Benchmark::BenchmarkFunction setup);
    void setTeardownFunction(Benchmark::BenchmarkFunction teardown);
    void enablePerformanceCounters(bool enable);

private:
    void warmUp();
    void measure();
    void runInThreads(const std::function<void()>& task);
    void printResults();
    void exportResults();

    void startPerfCounters();
    void stopPerfCounters();

    std::string name_;
    Benchmark::BenchmarkFunction function_;
    Benchmark::BenchmarkFunction setupFunction_;
    Benchmark::BenchmarkFunction teardownFunction_;
    int iterations_;
    int warmup_;
    int threads_;
    bool usePerformanceCounters_ = false;
    uint64_t prevCounter_ = 0;
    std::vector<long long> results_;
    std::vector<uint64_t> performanceCounters_;
    std::mutex mutex_;
};

#endif // BENCHMARK_H
