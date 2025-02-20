#include <chrono>
#include <iostream>
#include <thread>

#include "../include/metrics.hpp"

int main() {
    metrics::Registry& registry = metrics::Registry::instance();

    auto requests =
        registry.create<metrics::Counter>("requests_total", "Total number of requests processed");

    auto temperature =
        registry.create<metrics::Gauge>("temperature_celsius", "Current temperature in Celsius");

    auto latency = registry.create<metrics::Histogram>("request_latency_seconds",
                                                       std::vector<double>{0.1, 0.5, 1.0, 2.0, 5.0},
                                                       "Request latency distribution");

    auto memory = registry.create<metrics::Summary>(
        "memory_usage_bytes", std::vector<double>{0.5, 0.9, 0.99}, "Memory usage distribution");

    auto version = registry.create<metrics::Info>(
        "app_info", metrics::Labels{{"version", "1.0.0"}, {"build_date", "2024-02-06"}},
        "Application version information");

    for (int i = 0; i < 10; ++i) {
        requests->inc();
        temperature->set(20.0 + (rand() % 100) / 10.0);
        latency->observe((rand() % 1000) / 100.0);
        memory->observe(rand() % 1000000);

        std::cout << "Current metrics state:\n" << registry.serialize() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
