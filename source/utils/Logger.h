#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <mutex>

namespace rasterizer {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string prefix;
        switch (level) {
            case LogLevel::Debug:   prefix = "[DEBUG] "; break;
            case LogLevel::Info:    prefix = "[INFO]  "; break;
            case LogLevel::Warning: prefix = "[WARN]  "; break;
            case LogLevel::Error:   prefix = "[ERROR] "; break;
        }
        
        std::string fullMessage = prefix + message;
        std::cout << fullMessage << std::endl;
        
        // Store in history (max 100 messages)
        history_.push_back(fullMessage);
        if (history_.size() > 100) {
            history_.erase(history_.begin());
        }
    }
    
    const std::vector<std::string>& getHistory() const { return history_; }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        history_.clear();
    }
    
private:
    Logger() = default;
    std::vector<std::string> history_;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) do { std::ostringstream oss; oss << msg; rasterizer::Logger::instance().log(rasterizer::LogLevel::Debug, oss.str()); } while(0)
#define LOG_INFO(msg) do { std::ostringstream oss; oss << msg; rasterizer::Logger::instance().log(rasterizer::LogLevel::Info, oss.str()); } while(0)
#define LOG_WARN(msg) do { std::ostringstream oss; oss << msg; rasterizer::Logger::instance().log(rasterizer::LogLevel::Warning, oss.str()); } while(0)
#define LOG_ERROR(msg) do { std::ostringstream oss; oss << msg; rasterizer::Logger::instance().log(rasterizer::LogLevel::Error, oss.str()); } while(0)

} 
