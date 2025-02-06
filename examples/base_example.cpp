#include <chrono>
#include <iostream>
#include <thread>

#include "../include/metrics.hpp"

int main() {
    // Получаем экземпляр реестра метрик
    metrics::Registry& registry = metrics::Registry::instance();

    // Создаем счетчик
    auto requests =
        registry.create<metrics::Counter>("requests_total", "Total number of requests processed");

    // Создаем gauge
    auto temperature =
        registry.create<metrics::Gauge>("temperature_celsius", "Current temperature in Celsius");

    // Создаем гистограмму
    auto latency = registry.create<metrics::Histogram>("request_latency_seconds",
                                                       std::vector<double>{0.1, 0.5, 1.0, 2.0, 5.0},
                                                       "Request latency distribution");

    // Создаем summary
    auto memory = registry.create<metrics::Summary>(
        "memory_usage_bytes", std::vector<double>{0.5, 0.9, 0.99}, "Memory usage distribution");

    // Создаем info метрику
    auto version = registry.create<metrics::Info>(
        "app_info", metrics::Labels{{"version", "1.0.0"}, {"build_date", "2024-02-06"}},
        "Application version information");

    // Симулируем некоторую активность
    for (int i = 0; i < 10; ++i) {
        // Увеличиваем счетчик
        requests->inc();

        // Обновляем gauge
        temperature->set(20.0 + (rand() % 100) / 10.0);

        // Добавляем значение в гистограмму
        latency->observe((rand() % 1000) / 100.0);

        // Добавляем значение в summary
        memory->observe(rand() % 1000000);

        // Выводим текущее состояние метрик
        std::cout << "Current metrics state:\n" << registry.serialize() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
