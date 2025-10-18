#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
private:
    std::ofstream logfile;
    LogLevel min_level;
    std::mutex log_mutex;  // Thread-safe logging

    std::string get_timestamp() {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        return std::string(buffer);
    }

    std::string level_to_string(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG:   return "[DEBUG]";
        case LogLevel::INFO:    return "[INFO]";
        case LogLevel::WARNING: return "[WARN]";
        case LogLevel::ERROR:   return "[ERROR]";
        default:                return "[?]";
        }
    }

    // Private constructor for singleton
    Logger(const std::string& filename, LogLevel min_level)
        : min_level(min_level) {
        logfile.open(filename, std::ios::out | std::ios::app);
        if (logfile.is_open()) {
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
            logfile << "\n========================================\n";
            logfile << "Log started at " << buffer << "\n";
            logfile << "========================================\n";
            logfile.flush();
        }
    }

    // Delete copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    static Logger& get_instance(const std::string& filename = "game_log.txt",
        LogLevel level = LogLevel::DEBUG) {
        static Logger instance(filename, level);
        return instance;
    }

    ~Logger() {
        if (logfile.is_open()) {
            logfile << "========================================\n";
            logfile << "Log ended\n";
            logfile << "========================================\n\n";
            logfile.close();
        }
    }

    void log(LogLevel level, const std::string& message) {
        if (level < min_level) return;

        std::lock_guard<std::mutex> lock(log_mutex);

        std::stringstream ss;
        ss << get_timestamp() << " " << level_to_string(level) << " " << message;
        std::string full_message = ss.str();

        std::cout << full_message << std::endl;

        if (logfile.is_open()) {
            logfile << full_message << std::endl;
            logfile.flush();
        }
    }

    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    void warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

    // Set minimum log level at runtime
    void set_log_level(LogLevel level) { min_level = level; }
};

// Convenience macros (use anywhere)
#define LOG_DEBUG(msg)   Logger::get_instance().debug(msg)
#define LOG_INFO(msg)    Logger::get_instance().info(msg)
#define LOG_WARN(msg)    Logger::get_instance().warning(msg)
#define LOG_ERROR(msg)   Logger::get_instance().error(msg)