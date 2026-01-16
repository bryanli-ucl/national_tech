#pragma once

#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace utils {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
    private:
    LogLevel m_min_level;
    std::mutex m_mutex;
    bool m_show_timestamp;
    bool m_show_level;
    bool m_colorize;

    // ANSI 颜色代码
    static constexpr const char* RESET    = "\033[0m";
    static constexpr const char* GRAY     = "\033[90m";
    static constexpr const char* BLUE     = "\033[34m";
    static constexpr const char* GREEN    = "\033[32m";
    static constexpr const char* YELLOW   = "\033[33m";
    static constexpr const char* RED      = "\033[31m";
    static constexpr const char* BOLD_RED = "\033[1;31m";

    Logger()
    : m_min_level(LogLevel::INFO), m_show_timestamp(true), m_show_level(true), m_colorize(true) {}

    public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    // 删除拷贝构造和赋值
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    void setLevel(LogLevel level) { m_min_level = level; }
    void setShowTimestamp(bool show) { m_show_timestamp = show; }
    void setShowLevel(bool show) { m_show_level = show; }
    void setColorize(bool colorize) { m_colorize = colorize; }

    template <typename... Args>
    void trace(Args&&... args) {
        log(LogLevel::TRACE, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(Args&&... args) {
        log(LogLevel::DEBUG, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(Args&&... args) {
        log(LogLevel::INFO, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(Args&&... args) {
        log(LogLevel::WARN, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(Args&&... args) {
        log(LogLevel::ERROR, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(Args&&... args) {
        log(LogLevel::FATAL, std::forward<Args>(args)...);
    }

    // 特殊：分隔线
    void separator(char c = '=', int length = 60) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << std::string(length, c) << std::endl;
    }

    // 特殊：标题
    void section(const std::string& title) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "\n";
        separator('=');
        std::cout << "  " << title << std::endl;
        separator('=');
    }

    private:
    template <typename... Args>
    void log(LogLevel level, Args&&... args) {
        if (level < m_min_level) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream oss;

        // 时间戳
        if (m_show_timestamp) {
            auto now    = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms     = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
            1000;

            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time_t);
#else
            localtime_r(&time_t, &tm);
#endif

            if (m_colorize) oss << GRAY;
            oss << std::put_time(&tm, "%H:%M:%S");
            oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            if (m_colorize) oss << RESET;
            oss << " ";
        }

        // 日志级别
        if (m_show_level) {
            const char* color     = "";
            const char* level_str = "";

            switch (level) {
            case LogLevel::TRACE:
                color     = GRAY;
                level_str = "TRACE";
                break;
            case LogLevel::DEBUG:
                color     = BLUE;
                level_str = "DEBUG";
                break;
            case LogLevel::INFO:
                color     = GREEN;
                level_str = "INFO ";
                break;
            case LogLevel::WARN:
                color     = YELLOW;
                level_str = "WARN ";
                break;
            case LogLevel::ERROR:
                color     = RED;
                level_str = "ERROR";
                break;
            case LogLevel::FATAL:
                color     = BOLD_RED;
                level_str = "FATAL";
                break;
            }

            if (m_colorize) oss << color;
            oss << "[" << level_str << "]";
            if (m_colorize) oss << RESET;
            oss << " ";
        }

        // 消息内容
        ((oss << std::forward<Args>(args)), ...);

        std::cout << oss.str() << std::endl;
    }
};

// 全局便捷函数
inline Logger& log() { return Logger::instance(); }

} // namespace utils

// 便捷宏
#define LOG_TRACE(...) utils::log().trace(__VA_ARGS__)
#define LOG_DEBUG(...) utils::log().debug(__VA_ARGS__)
#define LOG_INFO(...) utils::log().info(__VA_ARGS__)
#define LOG_WARN(...) utils::log().warn(__VA_ARGS__)
#define LOG_ERROR(...) utils::log().error(__VA_ARGS__)
#define LOG_FATAL(...) utils::log().fatal(__VA_ARGS__)
#define LOG_SECTION(title) utils::log().section(title)
#define LOG_SEPARATOR() utils::log().separator()
