#include "../include/metrics.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace metrics {

namespace {
    std::string escape_string(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '"': result += "\\\""; break;
                default: result += c;
            }
        }
        return result;
    }

    void write_labels(std::ostringstream& out, const Labels& labels) {
        if (labels.empty()) return;
        
        out << "{";
        bool first = true;
        for (const auto& label : labels) {
            if (!first) out << ",";
            out << label.name << "=\"" << escape_string(label.value) << "\"";
            first = false;
        }
        out << "}";
    }
}

std::string Counter::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    if (!help_.empty()) {
        out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " counter\n";
    out << name_;
    write_labels(out, labels_);
    out << " " << std::fixed << std::setprecision(3) << value_ << "\n";
    
    return out.str();
}

std::string Gauge::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    if (!help_.empty()) {
        out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " gauge\n";
    out << name_;
    write_labels(out, labels_);
    out << " " << std::fixed << std::setprecision(3) << value_ << "\n";
    
    return out.str();
}

void Histogram::observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    sum_ += value;
    count_++;
    
    // Find appropriate bucket
    size_t bucket_index = buckets_.size();
    for (size_t i = 0; i < buckets_.size(); ++i) {
        if (value <= buckets_[i]) {
            bucket_index = i;
            break;
        }
    }
    bucket_counts_[bucket_index]++;
}

std::string Histogram::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    if (!help_.empty()) {
        out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " histogram\n";
    
    // Buckets
    uint64_t cumulative = 0;
    for (size_t i = 0; i < buckets_.size(); ++i) {
        cumulative += bucket_counts_[i];
        out << name_ << "_bucket";
        
        Labels bucket_labels = labels_;
        bucket_labels.push_back({"le", std::to_string(buckets_[i])});
        write_labels(out, bucket_labels);
        
        out << " " << cumulative << "\n";
    }
    
    // +Inf bucket
    cumulative += bucket_counts_[buckets_.size()];
    out << name_ << "_bucket";
    
    Labels inf_labels = labels_;
    inf_labels.push_back({"le", "+Inf"});
    write_labels(out, inf_labels);
    
    out << " " << cumulative << "\n";
    
    // Sum and count
    out << name_ << "_sum";
    write_labels(out, labels_);
    out << " " << sum_ << "\n";
    
    out << name_ << "_count";
    write_labels(out, labels_);
    out << " " << count_ << "\n";
    
    return out.str();
}

void Summary::cleanup_old_values() {
    auto now = std::chrono::steady_clock::now();
    auto it = std::remove_if(values_.begin(), values_.end(),
        [&](const TimedValue& tv) {
            return (now - tv.timestamp) > window_size_;
        });
    values_.erase(it, values_.end());
}

void Summary::observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanup_old_values();
    values_.push_back(TimedValue(value));
    sum_ += value;
    count_++;
}

std::string Summary::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    if (!help_.empty()) {
        out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " summary\n";
    
    if (!values_.empty()) {
        std::vector<double> current_values;
        current_values.reserve(values_.size());
        for (const auto& tv : values_) {
            current_values.push_back(tv.value);
        }
        std::sort(current_values.begin(), current_values.end());
        
        for (double q : quantiles_) {
            size_t index = static_cast<size_t>(q * (current_values.size() - 1));
            out << name_;
            
            Labels quantile_labels = labels_;
            quantile_labels.push_back({"quantile", std::to_string(q)});
            write_labels(out, quantile_labels);
            
            out << " " << std::fixed << std::setprecision(3) 
                << current_values[index] << "\n";
        }
    }
    
    out << name_ << "_sum";
    write_labels(out, labels_);
    out << " " << sum_ << "\n";
    
    out << name_ << "_count";
    write_labels(out, labels_);
    out << " " << count_ << "\n";
    
    return out.str();
}

std::string Info::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    if (!help_.empty()) {
        out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " info\n";
    out << name_;
    write_labels(out, labels_);
    out << " 1\n";
    
    return out.str();
}

std::string Registry::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    for (const auto& metric_pair : metrics_) {
        out << metric_pair.second->serialize();
    }
    
    return out.str();
}

} // namespace metrics 