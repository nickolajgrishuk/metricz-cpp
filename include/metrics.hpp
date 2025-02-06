#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace metrics {

// Metric types according to OpenMetrics specification
enum class MetricType {
    Counter,    // Monotonically increasing value
    Gauge,      // Arbitrary value that can go up and down
    Histogram,  // Distribution of values in buckets
    Summary,    // Statistical distribution
    Info        // Constant metadata
};

// Structure for storing labels
struct Label {
    std::string name;
    std::string value;

    Label(const std::string& n, const std::string& v) : name(n), value(v) {}
};

using Labels = std::vector<Label>;

// Base class for all metrics
class Metric {
public:
    explicit Metric(const std::string& name, const std::string& help = "",
                    const Labels& labels = {})
        : name_(name), help_(help), labels_(labels) {}

    virtual ~Metric() = default;

    virtual std::string serialize() const = 0;
    virtual MetricType type() const = 0;

    const std::string& name() const {
        return name_;
    }
    const std::string& help() const {
        return help_;
    }
    const Labels& labels() const {
        return labels_;
    }

    void set_labels(const Labels& labels) {
        std::lock_guard<std::mutex> lock(mutex_);
        labels_ = labels;
    }

protected:
    std::string name_;
    std::string help_;
    Labels labels_;
    mutable std::mutex mutex_;
};

// Counter - monotonically increasing metric
class Counter : public Metric {
public:
    explicit Counter(const std::string& name, const std::string& help = "",
                     const Labels& labels = {})
        : Metric(name, help, labels), value_(0.0) {}

    void inc(double v = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ += v;
    }

    double value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    MetricType type() const override {
        return MetricType::Counter;
    }
    std::string serialize() const override;

private:
    double value_;
};

// Gauge - metric with arbitrary value
class Gauge : public Metric {
public:
    explicit Gauge(const std::string& name, const std::string& help = "", const Labels& labels = {})
        : Metric(name, help, labels), value_(0.0) {}

    void set(double v) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = v;
    }

    void inc(double v = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ += v;
    }

    void dec(double v = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ -= v;
    }

    double value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    MetricType type() const override {
        return MetricType::Gauge;
    }
    std::string serialize() const override;

private:
    double value_;
};

// Histogram - distribution of values in buckets
class Histogram : public Metric {
public:
    explicit Histogram(const std::string& name,
                       const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25,
                                                             0.5, 1, 2.5, 5, 10},
                       const std::string& help = "", const Labels& labels = {})
        : Metric(name, help, labels), buckets_(buckets), count_(0), sum_(0) {
        bucket_counts_.resize(buckets.size() + 1, 0);  // +1 for +Inf bucket
    }

    void observe(double value);
    MetricType type() const override {
        return MetricType::Histogram;
    }
    std::string serialize() const override;

private:
    std::vector<double> buckets_;
    std::vector<uint64_t> bucket_counts_;
    uint64_t count_;
    double sum_;
};

// Summary - quantile distribution
class Summary : public Metric {
public:
    struct Quantile {
        double quantile;
        double value;
        Quantile(double q, double v) : quantile(q), value(v) {}
    };

    explicit Summary(const std::string& name,
                     const std::vector<double>& quantiles = {0.5, 0.9, 0.99},
                     const std::string& help = "", const Labels& labels = {})
        : Metric(name, help, labels), quantiles_(quantiles), count_(0), sum_(0) {
        // Add window for sliding average (1 minute)
        window_size_ = std::chrono::minutes(1);
    }

    void observe(double value);
    MetricType type() const override {
        return MetricType::Summary;
    }
    std::string serialize() const override;

private:
    void cleanup_old_values();

    std::vector<double> quantiles_;
    struct TimedValue {
        double value;
        std::chrono::steady_clock::time_point timestamp;
        TimedValue(double v) : value(v), timestamp(std::chrono::steady_clock::now()) {}
    };
    std::vector<TimedValue> values_;
    uint64_t count_;
    double sum_;
    std::chrono::steady_clock::duration window_size_;
};

// Info - constant metadata metric
class Info : public Metric {
public:
    explicit Info(const std::string& name, const Labels& labels = {}, const std::string& help = "")
        : Metric(name, help, labels) {}

    MetricType type() const override {
        return MetricType::Info;
    }
    std::string serialize() const override;
};

// Registry for managing metrics
class Registry {
public:
    static Registry& instance() {
        static Registry instance;
        return instance;
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> create(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto metric = std::make_shared<T>(std::forward<Args>(args)...);
        metrics_[metric->name()] = metric;
        return metric;
    }

    std::string serialize() const;

    // Get metric by name
    std::shared_ptr<Metric> get(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        return it != metrics_.end() ? it->second : nullptr;
    }

    // Remove metric by name
    bool remove(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_.erase(name) > 0;
    }

    // Clear all metrics
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_.clear();
    }

private:
    Registry() = default;
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Metric>> metrics_;
};

}  // namespace metrics