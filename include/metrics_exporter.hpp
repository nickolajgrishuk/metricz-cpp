#pragma once

#include "metrics.hpp"
#include "httplib.h"
#include <string>
#include <memory>
#include <thread>

namespace metrics {

// Base class for all metric exporters
class MetricsExporter {
public:
    explicit MetricsExporter(Registry& registry) : registry_(registry) {}
    virtual ~MetricsExporter() = default;
    
    virtual void start() = 0;
    virtual void stop() = 0;

protected:
    Registry& registry_;
};

// HTTP exporter for metrics
class HttpExporter : public MetricsExporter {
public:
    HttpExporter(Registry& registry, const std::string& host = "0.0.0.0", int port = 9000)
        : MetricsExporter(registry), host_(host), port_(port), running_(false) {}
    
    ~HttpExporter() {
        stop();
    }
    
    void start() override {
        if (running_) return;
        
        server_.Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(registry_.serialize(), "text/plain");
        });
        
        running_ = true;
        server_thread_ = std::thread([this]() {
            server_.listen(host_.c_str(), port_);
        });
    }
    
    void stop() override {
        if (!running_) return;
        
        server_.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        running_ = false;
    }

private:
    std::string host_;
    int port_;
    bool running_;
    httplib::Server server_;
    std::thread server_thread_;
};

#ifdef _WIN32
// Windows does not support Unix sockets
class UnixSocketExporter : public MetricsExporter {
public:
    UnixSocketExporter(Registry&, const std::string&)
        : MetricsExporter(registry_) {
        throw std::runtime_error("Unix socket exporter is not supported on Windows");
    }
    
    void start() override {}
    void stop() override {}
};
#else
// Unix socket exporter for metrics
class UnixSocketExporter : public MetricsExporter {
public:
    UnixSocketExporter(Registry& registry, const std::string& socket_path = "/tmp/metrics.sock")
        : MetricsExporter(registry), socket_path_(socket_path), running_(false) {
        // Remove old socket if exists
        std::remove(socket_path_.c_str());
    }
    
    ~UnixSocketExporter() {
        stop();
        std::remove(socket_path_.c_str());
    }
    
    void start() override {
        if (running_) return;
        
        server_.Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(registry_.serialize(), "text/plain");
        });
        
        running_ = true;
        server_thread_ = std::thread([this]() {
            #ifdef CPPHTTPLIB_USE_UNIX_DOMAIN_SOCKET
            server_.set_mount_point("/");
            if (!server_.bind_to_unix_domain_socket(socket_path_.c_str())) {
                std::cerr << "Failed to bind to unix domain socket: " << socket_path_ << std::endl;
                return;
            }
            server_.listen();
            #endif
        });
    }
    
    void stop() override {
        if (!running_) return;
        
        server_.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        running_ = false;
    }

private:
    std::string socket_path_;
    bool running_;
    httplib::Server server_;
    std::thread server_thread_;
};
#endif

} // namespace metrics 