#include "benchmark.h"

namespace {

auto clampNonNegative(int value) -> int {
    return std::max(value, 0);
}

#if defined(__i386__) || defined(__x86_64__)
auto readPerfCounter() -> std::uint64_t {
    unsigned int low = 0;
    unsigned int high = 0;
    __asm__ volatile(
        "rdpmc\n\t"
        "mov %%eax, %0\n\t"
        "mov %%edx, %1\n\t"
        : "=r"(low), "=r"(high)
        :
        : "%eax", "%edx");
    return (static_cast<std::uint64_t>(high) << 32U) | low;
}

auto perfCounterBaseline() -> std::uint64_t& {
    static thread_local std::uint64_t previousCounter = 0;
    return previousCounter;
}
#endif

} // namespace

Benchmark::Benchmark(std::string name, BenchmarkFunction fn, int iterations, int warmup)
    : name_(std::move(name)),
      function_(std::move(fn)),
      iterations_(clampNonNegative(iterations)),
      warmup_(clampNonNegative(warmup)) {}

void Benchmark::run() {
    resetMeasurements();
    warmUp();
    measure();
    printResults();
}

void Benchmark::setSetupFunction(BenchmarkFunction setup) {
    setupFunction_ = std::move(setup);
}

void Benchmark::setTeardownFunction(BenchmarkFunction teardown) {
    teardownFunction_ = std::move(teardown);
}

void Benchmark::enablePerformanceCounters(bool enable) {
    usePerformanceCounters_ = enable && supportsPerformanceCounters();
    if (enable && !usePerformanceCounters_) {
        std::cerr << "Performance counters are only available on x86/x86_64 builds." << std::endl;
    }
}

void Benchmark::resetMeasurements() {
    const auto sampleCount = static_cast<size_t>(iterations_);
    results_.clear();
    performanceCounters_.clear();
    results_.reserve(sampleCount);
    performanceCounters_.reserve(sampleCount);
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

        const auto start = std::chrono::high_resolution_clock::now();
        if (usePerformanceCounters_) {
            startPerfCounters();
        }
        function_();
        if (usePerformanceCounters_) {
            stopPerfCounters();
        }
        const auto end = std::chrono::high_resolution_clock::now();

        if (teardownFunction_) teardownFunction_();

        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        results_.push_back(duration);
    }
}

void Benchmark::printResults() {
    if (results_.empty()) {
        std::cout << "Benchmark: " << name_ << '\n'
                  << "No measurements were collected." << std::endl;
        return;
    }

    const auto mean = std::accumulate(results_.begin(), results_.end(), 0LL) / static_cast<long long>(results_.size());
    const auto variance = std::accumulate(results_.begin(), results_.end(), 0LL,
        [mean](long long sum, long long value) {
            return sum + (value - mean) * (value - mean);
        }) / static_cast<long long>(results_.size());

    const auto stddev = std::sqrt(static_cast<double>(variance));

    std::cout << "Benchmark: " << name_ << std::endl;
    std::cout << "Iterations: " << iterations_ << std::endl;
    std::cout << "Samples: " << results_.size() << std::endl;
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

void Benchmark::startPerfCounters() {
#if defined(__i386__) || defined(__x86_64__)
    perfCounterBaseline() = readPerfCounter();
#endif
}

void Benchmark::stopPerfCounters() {
#if defined(__i386__) || defined(__x86_64__)
    const auto currentCounter = readPerfCounter();
    performanceCounters_.push_back(currentCounter - perfCounterBaseline());
#endif
}

bool Benchmark::supportsPerformanceCounters() {
#if defined(__i386__) || defined(__x86_64__)
    return true;
#else
    return false;
#endif
}

MultiThreadedBenchmark::MultiThreadedBenchmark(
    std::string name,
    Benchmark::BenchmarkFunction fn,
    int iterations,
    int warmup,
    int threads)
    : name_(std::move(name)),
      function_(std::move(fn)),
      iterations_(clampNonNegative(iterations)),
      warmup_(clampNonNegative(warmup)),
      threads_(threads > 0 ? threads : 1) {}

void MultiThreadedBenchmark::run() {
    resetMeasurements();
    warmUp();
    measure();
    printResults();
}

void MultiThreadedBenchmark::setSetupFunction(Benchmark::BenchmarkFunction setup) {
    setupFunction_ = std::move(setup);
}

void MultiThreadedBenchmark::setTeardownFunction(Benchmark::BenchmarkFunction teardown) {
    teardownFunction_ = std::move(teardown);
}

void MultiThreadedBenchmark::enablePerformanceCounters(bool enable) {
    usePerformanceCounters_ = enable && supportsPerformanceCounters();
    if (enable && !usePerformanceCounters_) {
        std::cerr << "Performance counters are only available on x86/x86_64 builds." << std::endl;
    }
}

void MultiThreadedBenchmark::resetMeasurements() {
    const auto sampleCount = static_cast<size_t>(iterations_) * static_cast<size_t>(threads_);
    results_.clear();
    performanceCounters_.clear();
    results_.reserve(sampleCount);
    performanceCounters_.reserve(sampleCount);
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
        runInThreads([this] {
            if (setupFunction_) setupFunction_();

            const auto start = std::chrono::high_resolution_clock::now();
            if (usePerformanceCounters_) {
                startPerfCounters();
            }
            function_();
            if (usePerformanceCounters_) {
                stopPerfCounters();
            }
            const auto end = std::chrono::high_resolution_clock::now();

            if (teardownFunction_) teardownFunction_();

            const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            std::lock_guard<std::mutex> lock(mutex_);
            results_.push_back(duration);
        });
    }
}

void MultiThreadedBenchmark::runInThreads(const std::function<void()>& task) {
    std::vector<std::thread> threadPool;
    threadPool.reserve(static_cast<size_t>(threads_));

    for (int t = 0; t < threads_; ++t) {
        threadPool.emplace_back(task);
    }
    for (auto& thread : threadPool) {
        thread.join();
    }
}

void MultiThreadedBenchmark::printResults() {
    if (results_.empty()) {
        std::cout << "Benchmark: " << name_ << '\n'
                  << "No measurements were collected." << std::endl;
        return;
    }

    const auto mean = std::accumulate(results_.begin(), results_.end(), 0LL) / static_cast<long long>(results_.size());
    const auto variance = std::accumulate(results_.begin(), results_.end(), 0LL,
        [mean](long long sum, long long value) {
            return sum + (value - mean) * (value - mean);
        }) / static_cast<long long>(results_.size());

    const auto stddev = std::sqrt(static_cast<double>(variance));

    std::cout << "Benchmark: " << name_ << std::endl;
    std::cout << "Iterations: " << iterations_ << std::endl;
    std::cout << "Threads: " << threads_ << std::endl;
    std::cout << "Samples: " << results_.size() << std::endl;
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

void MultiThreadedBenchmark::startPerfCounters() {
#if defined(__i386__) || defined(__x86_64__)
    perfCounterBaseline() = readPerfCounter();
#endif
}

void MultiThreadedBenchmark::stopPerfCounters() {
#if defined(__i386__) || defined(__x86_64__)
    const auto currentCounter = readPerfCounter();
    std::lock_guard<std::mutex> lock(mutex_);
    performanceCounters_.push_back(currentCounter - perfCounterBaseline());
#endif
}

bool MultiThreadedBenchmark::supportsPerformanceCounters() {
#if defined(__i386__) || defined(__x86_64__)
    return true;
#else
    return false;
#endif
}
