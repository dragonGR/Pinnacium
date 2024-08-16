#include "benchmark.h"

Benchmark::Benchmark(std::string name, BenchmarkFunction fn, int iterations, int warmup)
    : name_(std::move(name)), function_(std::move(fn)), iterations_(iterations), warmup_(warmup) {}

void Benchmark::run() {
    warmUp();
    measure();
    printResults();
    exportResults();
}

void Benchmark::setSetupFunction(BenchmarkFunction setup) {
    setupFunction_ = std::move(setup);
}

void Benchmark::setTeardownFunction(BenchmarkFunction teardown) {
    teardownFunction_ = std::move(teardown);
}

void Benchmark::enablePerformanceCounters(bool enable) {
    usePerformanceCounters_ = enable;
}

void Benchmark::warmUp() {
    for (int i = 0; i < warmup_; ++i) {
        if (setupFunction_) setupFunction_();
        function_();
        if (teardownFunction_) teardownFunction_();
    }
}

void Benchmark::measure() {
    for (int i = 0; i < iterations_; ++i) {
        if (setupFunction_) setupFunction_();

        auto start = std::chrono::high_resolution_clock::now();
        if (usePerformanceCounters_) {
            startPerfCounters();
        }
        function_();
        if (usePerformanceCounters_) {
            stopPerfCounters();
        }
        auto end = std::chrono::high_resolution_clock::now();

        if (teardownFunction_) teardownFunction_();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        results_.push_back(duration);
    }
}

void Benchmark::printResults() {
    auto mean = std::accumulate(results_.begin(), results_.end(), 0LL) / results_.size();
    auto variance = std::accumulate(results_.begin(), results_.end(), 0LL,
        [mean](long long sum, long long value) {
            return sum + (value - mean) * (value - mean);
        }
    ) / results_.size();

    auto stddev = std::sqrt(variance);

    std::cout << "Benchmark: " << name_ << std::endl;
    std::cout << "Iterations: " << iterations_ << std::endl;
    std::cout << "Mean: " << mean << " ns" << std::endl;
    std::cout << "Stddev: " << stddev << " ns" << std::endl;
    std::cout << "Min: " << *std::min_element(results_.begin(), results_.end()) << " ns" << std::endl;
    std::cout << "Max: " << *std::max_element(results_.begin(), results_.end()) << " ns" << std::endl;

    if (usePerformanceCounters_) {
        std::cout << "Performance Counters:" << std::endl;
        for (const auto& counter : performanceCounters_) {
            std::cout << "Counter Value: " << counter << std::endl;
        }
    }

    std::cout << "=========================" << std::endl;
}

void Benchmark::exportResults() {
    std::ofstream file(name_ + "_results.csv");
    file << "Iteration,Duration (ns)";
    if (usePerformanceCounters_) {
        file << ",Performance Counter";
    }
    file << "\n";
    for (size_t i = 0; i < results_.size(); ++i) {
        file << i + 1 << "," << results_[i];
        if (usePerformanceCounters_ && i < performanceCounters_.size()) {
            file << "," << performanceCounters_[i];
        }
        file << "\n";
    }
    file.close();
    std::cout << "Results exported to " << name_ << "_results.csv" << std::endl;
}

void Benchmark::startPerfCounters() {
    unsigned int low, high;
    __asm__ volatile (
        "rdpmc\n\t"
        "mov %%eax, %0\n\t"
        "mov %%edx, %1\n\t"
        : "=r" (low), "=r" (high)
        :
        : "%eax", "%edx"
    );
    prevCounter_ = ((uint64_t)high << 32) | low;
}

void Benchmark::stopPerfCounters() {
    uint64_t currentCounter;
    __asm__ volatile ("rdpmc" : "=A" (currentCounter) : : "memory");
    performanceCounters_.push_back(currentCounter - prevCounter_);
}

MultiThreadedBenchmark::MultiThreadedBenchmark(std::string name, Benchmark::BenchmarkFunction fn, int iterations, int warmup, int threads)
    : name_(std::move(name)), function_(std::move(fn)), iterations_(iterations), warmup_(warmup), threads_(threads) {}

void MultiThreadedBenchmark::run() {
    warmUp();
    measure();
    printResults();
    exportResults();
}

void MultiThreadedBenchmark::setSetupFunction(Benchmark::BenchmarkFunction setup) {
    setupFunction_ = std::move(setup);
}

void MultiThreadedBenchmark::setTeardownFunction(Benchmark::BenchmarkFunction teardown) {
    teardownFunction_ = std::move(teardown);
}

void MultiThreadedBenchmark::enablePerformanceCounters(bool enable) {
    usePerformanceCounters_ = enable;
}

void MultiThreadedBenchmark::warmUp() {
    for (int i = 0; i < warmup_; ++i) {
        runInThreads([this] {
            if (setupFunction_) setupFunction_();
            function_();
            if (teardownFunction_) teardownFunction_();
        });
    }
}

void MultiThreadedBenchmark::measure() {
    for (int i = 0; i < iterations_; ++i) {
        runInThreads([this, i] {
            if (setupFunction_) setupFunction_();

            auto start = std::chrono::high_resolution_clock::now();
            if (usePerformanceCounters_) {
                startPerfCounters();
            }
            function_();
            if (usePerformanceCounters_) {
                stopPerfCounters();
            }
            auto end = std::chrono::high_resolution_clock::now();

            if (teardownFunction_) teardownFunction_();

            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                results_.push_back(duration);
            }
        });
    }
}

void MultiThreadedBenchmark::runInThreads(const std::function<void()>& task) {
    std::vector<std::thread> threadPool;
    for (int t = 0; t < threads_; ++t) {
        threadPool.emplace_back(task);
    }
    for (auto& th : threadPool) {
        th.join();
    }
}

void MultiThreadedBenchmark::printResults() {
    auto mean = std::accumulate(results_.begin(), results_.end(), 0LL) / results_.size();
    auto variance = std::accumulate(results_.begin(), results_.end(), 0LL,
        [mean](long long sum, long long value) {
            return sum + (value - mean) * (value - mean);
        }
    ) / results_.size();

    auto stddev = std::sqrt(variance);

    std::cout << "Benchmark: " << name_ << std::endl;
    std::cout << "Iterations: " << iterations_ << std::endl;
    std::cout << "Mean: " << mean << " ns" << std::endl;
    std::cout << "Stddev: " << stddev << " ns" << std::endl;
    std::cout << "Min: " << *std::min_element(results_.begin(), results_.end()) << " ns" << std::endl;
    std::cout << "Max: " << *std::max_element(results_.begin(), results_.end()) << " ns" << std::endl;

    if (usePerformanceCounters_) {
        std::cout << "Performance Counters:" << std::endl;
        for (const auto& counter : performanceCounters_) {
            std::cout << "Counter Value: " << counter << std::endl;
        }
    }

    std::cout << "=========================" << std::endl;
}

void MultiThreadedBenchmark::exportResults() {
    std::ofstream file(name_ + "_results.csv");
    file << "Iteration,Duration (ns)";
    if (usePerformanceCounters_) {
        file << ",Performance Counter";
    }
    file << "\n";
    for (size_t i = 0; i < results_.size(); ++i) {
        file << i + 1 << "," << results_[i];
        if (usePerformanceCounters_ && i < performanceCounters_.size()) {
            file << "," << performanceCounters_[i];
        }
        file << "\n";
    }
    file.close();
    std::cout << "Results exported to " << name_ << "_results.csv" << std::endl;
}

void MultiThreadedBenchmark::startPerfCounters() {
    __asm__ volatile ("rdpmc" : "=A" (prevCounter_) : : "memory");
}

void MultiThreadedBenchmark::stopPerfCounters() {
    uint64_t currentCounter;
    __asm__ volatile ("rdpmc" : "=A" (currentCounter) : : "memory");
    performanceCounters_.push_back(currentCounter - prevCounter_);
}