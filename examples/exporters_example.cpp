#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "../include/metrics.hpp"
#include "../include/metrics_exporter.hpp"

int main(int argc, char* argv[]) {
    std::string host = "0.0.0.0";
    int port = 9000;
    std::string unix_socket = "/tmp/metrics.sock";

    // Разбор аргументов командной строки
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--unix-socket" && i + 1 < argc) {
            unix_socket = argv[++i];
        }
    }

    // Создаем реестр метрик
    metrics::Registry& registry = metrics::Registry::instance();

    // Создаем метрики
    auto requests =
        registry.create<metrics::Counter>("http_requests_total", "Total number of HTTP requests");

    auto cpu_usage =
        registry.create<metrics::Gauge>("cpu_usage_percent", "Current CPU usage in percent");

    // Создаем HTTP экспортер с указанными параметрами
    metrics::HttpExporter http_exporter(registry, host, port);
    http_exporter.start();

    // Создаем Unix Socket экспортер
    metrics::UnixSocketExporter unix_exporter(registry, unix_socket);
    unix_exporter.start();

    std::cout << "Metrics available at:\n";
    std::cout << "  - HTTP: http://" << host << ":" << port << "/metrics\n";
    std::cout << "  - Unix Socket: " << unix_socket << "\n";

    // Симулируем работу приложения
    while (true) {
        requests->inc();
        cpu_usage->set(static_cast<double>(rand()) / RAND_MAX * 100);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}