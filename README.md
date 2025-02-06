# metricz-cpp

A lightweight, modern C++11 metrics library that implements the OpenMetrics format, designed for easy integration and high performance. Perfect for both enterprise applications and resource-constrained embedded systems.

[![Build Status](https://github.com/nickolajgrishuk/metricz-cpp/workflows/CI/badge.svg)](https://github.com/nickolajgrishuk/metricz-cpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Features

- 🚀 High-performance, thread-safe implementation
- 📊 Support for all OpenMetrics types:
  - Counter (monotonically increasing value)
  - Gauge (arbitrary value)
  - Histogram (value distribution)
  - Summary (quantile distribution)
  - Info (constant metadata)
- 🏷️ Full labels support
- 🌐 Multiple export options:
  - HTTP endpoint
  - Unix Domain Socket
- 🔒 Thread-safe operations
- ⚡ Zero-allocation fast path
- 🎯 Simple, modern API
- 💡 Lightweight design:
  - Small memory footprint
  - Minimal external dependencies
  - Suitable for embedded systems
  - Configurable feature set
  - Header-only option available

## Embedded Systems Support

metricz-cpp is designed with embedded systems in mind:
- Minimal heap allocations
- Small binary size (~50KB for basic setup)
- Configurable memory limits
- Optional features can be disabled to reduce footprint
- No dynamic memory allocation in critical paths
- Works on systems with limited resources (e.g., ARM Cortex-M, RISC-V)

## TODO

The following features are planned for future releases:

### Data Processing
- [ ] Metric aggregation support
- [ ] Time series support
- [ ] Metric persistence (save/load functionality)
- [ ] Advanced metric validation (names and labels)

### Export & Integration
- [ ] Additional export formats
  - [ ] JSON
  - [ ] XML
  - [ ] Custom format plugins
- [ ] Push model support for metric forwarding
- [ ] Metric filtering during export
- [ ] Metric grouping capabilities

### Advanced Features
- [ ] Metric aggregation rules
- [ ] Custom aggregation functions
- [ ] Dynamic label management
- [ ] High-cardinality optimization

Want to contribute to any of these features? Check out our [Contributing](#contributing) section!

## Installation

### Using CMake

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

### Requirements

- C++11 compatible compiler
- CMake 3.10 or higher
- pthread (on Unix systems)
- cpp-httplib (automatically downloaded during build)
  - Version: 0.12.0 or higher
  - SSL support (optional)
  - ZLIB support (optional)

Note: cpp-httplib is automatically downloaded and configured by CMake during the build process. No manual installation is required.

## Quick Start

```cpp
#include <metrics.hpp>
#include <metrics_exporter.hpp>
#include <iostream>

int main() {
    // Get metrics registry
    auto& registry = metrics::Registry::instance();
    
    // Create metrics
    auto requests = registry.create<metrics::Counter>("http_requests_total",
        "Total HTTP requests",
        metrics::Labels{{"method", "GET"}, {"path", "/api"}});
    
    auto temperature = registry.create<metrics::Gauge>("temperature_celsius",
        "Current temperature");
    
    // Create HTTP exporter
    metrics::HttpExporter exporter(registry, "0.0.0.0", 9000);
    exporter.start();
    
    // Use metrics
    requests->inc();
    temperature->set(23.5);
    
    std::cout << "Metrics available at: http://localhost:9000/metrics" << std::endl;
    
    // Keep the application running
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

## Metric Types

### Counter

Use for monotonically increasing values:

```cpp
auto requests = registry.create<metrics::Counter>("requests_total",
    "Total requests processed");
requests->inc();  // Increment by 1
requests->inc(5); // Increment by 5
```

### Gauge

Use for arbitrary values that can go up and down:

```cpp
auto temperature = registry.create<metrics::Gauge>("temperature_celsius");
temperature->set(23.5);  // Set value
temperature->inc(1.5);   // Increase
temperature->dec(0.5);   // Decrease
```

### Histogram

Use for measuring value distributions:

```cpp
auto latency = registry.create<metrics::Histogram>("request_latency_seconds",
    std::vector<double>{0.1, 0.5, 1.0, 2.0, 5.0});
latency->observe(0.7);  // Record a value
```

### Summary

Use for quantile calculations:

```cpp
auto memory = registry.create<metrics::Summary>("memory_usage_bytes",
    std::vector<double>{0.5, 0.9, 0.99});
memory->observe(1024);  // Record a value
```

### Info

Use for constant metadata:

```cpp
auto version = registry.create<metrics::Info>("app_info",
    metrics::Labels{
        {"version", "1.0.0"},
        {"build_date", "2024-02-06"}
    });
```

## Labels

All metric types support labels:

```cpp
auto requests = registry.create<metrics::Counter>("http_requests_total",
    "Total HTTP requests",
    metrics::Labels{
        {"method", "GET"},
        {"path", "/api"},
        {"status", "200"}
    });

// Update labels later
requests->set_labels(metrics::Labels{
    {"method", "POST"},
    {"path", "/api/users"},
    {"status", "201"}
});
```

## Exporters

### HTTP Exporter

```cpp
metrics::HttpExporter exporter(registry, "0.0.0.0", 9000);
exporter.start();
```

### Unix Socket Exporter

```cpp
metrics::UnixSocketExporter exporter(registry, "/tmp/metrics.sock");
exporter.start();
```

## Best Practices

1. **Naming Conventions**
   - Use snake_case for metric names
   - Add units as suffix (e.g., `_seconds`, `_bytes`, `_total`)
   - Use descriptive help text

2. **Labels**
   - Keep cardinality low (avoid using high-cardinality values like IDs)
   - Use consistent label names across metrics
   - Prefer static labels for low-cardinality dimensions

3. **Performance**
   - Reuse metric instances instead of creating new ones
   - Use appropriate metric types for your use case
   - Consider using labels instead of creating multiple metrics

## Thread Safety

All operations in the library are thread-safe. You can safely:
- Create metrics from multiple threads
- Update metric values concurrently
- Export metrics while they're being updated

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details. 