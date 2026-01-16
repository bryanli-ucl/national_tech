#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace utils {

/**
 * @brief Log level enumeration
 * Defines the severity levels for logging messages
 */
enum class LogLevel {
    TRACE, // Detailed trace information
    DEBUG, // Debug information
    INFO,  // General information
    WARN,  // Warning messages
    ERROR, // Error messages
    FATAL  // Fatal error messages
};

/**
 * @brief Thread-safe asynchronous logger class
 *
 * Implements a singleton pattern logger with the following features:
 * - Asynchronous logging using a background worker thread
 * - Console and file output support
 * - Configurable log levels, timestamps, and colorization
 * - Thread-safe operations
 */
class Logger {
    private:
    // Configuration flags
    LogLevel m_min_level;  // Minimum log level to output
    bool m_show_timestamp; // Whether to show timestamps
    bool m_show_level;     // Whether to show log level
    bool m_colorize;       // Whether to colorize console output

    // Asynchronous logging queue
    std::queue<std::string> m_log_queue; // Queue for log messages
    std::mutex m_queue_mutex;            // Mutex for queue access
    std::condition_variable m_cv;        // Condition variable for worker thread
    std::thread m_worker_thread;         // Background worker thread
    std::atomic<bool> m_running;         // Flag indicating if logger is running
    std::atomic<bool> m_worker_finished; // Flag indicating worker thread finished

    // File logging
    std::ofstream m_file_stream;     // File output stream
    std::atomic<bool> m_log_to_file; // Flag to enable/disable file logging
    std::string m_log_file_path;     // Current log file path
    std::mutex m_file_mutex;         // Mutex for file operations

    // Timestamp caching for performance optimization
    struct TimeCache {
        std::atomic<int64_t> last_second{ 0 }; // Last cached second
        std::string cached_time_str;           // Cached time string
        std::mutex mutex;                      // Mutex for cache access
    } m_time_cache;

    // ANSI color codes for console output
    static constexpr const char* RESET    = "\033[0m";
    static constexpr const char* GRAY     = "\033[90m";
    static constexpr const char* BLUE     = "\033[34m";
    static constexpr const char* GREEN    = "\033[32m";
    static constexpr const char* YELLOW   = "\033[33m";
    static constexpr const char* RED      = "\033[31m";
    static constexpr const char* BOLD_RED = "\033[1;31m";

    // Private constructor for singleton pattern
    Logger();

    // Delete copy constructor and assignment operator
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    // Destructor
    ~Logger();

    /**
     * @brief Worker thread function to process log queue
     * Continuously processes log messages from the queue and outputs them
     */
    auto process_logs() -> void;

    /**
     * @brief Remove ANSI color codes from string
     * @param str Input string with color codes
     * @return String without color codes
     */
    auto removeColorCodes(const std::string& str) -> std::string;

    /**
     * @brief Format current timestamp
     * @return Formatted timestamp string with milliseconds
     */
    auto formatTimestamp() -> std::string;

    /**
     * @brief Generate log filename based on current timestamp
     * @return Generated log filename (e.g., "logs/2025-01-16_14-30-45.log")
     */
    auto generateLogFilename() -> std::string;

    /**
     * @brief Create directory if it doesn't exist
     * @param path Directory path to create
     * @return true if directory exists or was created successfully
     */
    auto createDirectory(const std::string& path) -> bool;

    public:
    /**
     * @brief Get singleton instance of Logger
     * @return Reference to the Logger instance
     */
    static auto instance() -> Logger&;

    // Configuration setters
    void setLevel(LogLevel level) { m_min_level = level; }
    void setShowTimestamp(bool show) { m_show_timestamp = show; }
    void setShowLevel(bool show) { m_show_level = show; }
    void setColorize(bool colorize) { m_colorize = colorize; }

    /**
     * @brief Enable file logging
     * Creates log directory and opens log file
     * @return true if file logging was enabled successfully
     */
    auto enableFileLogging() -> bool;

    /**
     * @brief Disable file logging
     * Closes the log file
     */
    auto disableFileLogging() -> void;

    /**
     * @brief Get current log file path
     * @return Log file path
     */
    auto getLogFilePath() const -> std::string;

    /**
     * @brief Flush all pending log messages
     * Waits for the queue to be processed and flushes file stream
     */
    auto flush() -> void;

    /**
     * @brief Output a separator line
     * @param c Character to use for separator
     * @param length Length of separator line
     */
    auto separator(char c = '=', int length = 60) -> void;

    /**
     * @brief Output a section header
     * @param title Section title
     */
    auto section(const std::string& title) -> void;

    // Logging methods for different levels
    template <typename... Args>
    void trace(Args&&... args) { log(LogLevel::TRACE, std::forward<Args>(args)...); }

    template <typename... Args>
    void debug(Args&&... args) { log(LogLevel::DEBUG, std::forward<Args>(args)...); }

    template <typename... Args>
    void info(Args&&... args) { log(LogLevel::INFO, std::forward<Args>(args)...); }

    template <typename... Args>
    void warn(Args&&... args) { log(LogLevel::WARN, std::forward<Args>(args)...); }

    template <typename... Args>
    void error(Args&&... args) { log(LogLevel::ERROR, std::forward<Args>(args)...); }

    template <typename... Args>
    void fatal(Args&&... args) { log(LogLevel::FATAL, std::forward<Args>(args)...); }

    private:
    /**
     * @brief Enqueue log message to processing queue
     * @param log_msg Log message to enqueue
     */
    auto enqueueLog(const std::string& log_msg) -> void;

    /**
     * @brief Core logging function
     * @tparam Args Variadic template arguments
     * @param level Log level
     * @param args Arguments to log
     */
    template <typename... Args>
    void log(LogLevel level, Args&&... args) {
        // Skip if below minimum level
        if (level < m_min_level) return;

        std::ostringstream oss;

        // Add timestamp if enabled
        if (m_show_timestamp) {
            oss << formatTimestamp() << " ";
        }

        // Add log level if enabled
        if (m_show_level) {
            const char* color     = "";
            const char* level_str = "";

            // Determine color and level string based on log level
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

        // Fold expression to concatenate all arguments
        ((oss << std::forward<Args>(args)), ...);

        // Enqueue the formatted log message
        enqueueLog(oss.str());
    }
};

/**
 * @brief Convenience function to get logger instance
 * @return Reference to Logger singleton
 */
inline Logger& log() { return Logger::instance(); }

} // namespace utils

// Convenience macros for logging
#define LOG_TRACE(...) utils::log().trace(__VA_ARGS__)
#define LOG_DEBUG(...) utils::log().debug(__VA_ARGS__)
#define LOG_INFO(...) utils::log().info(__VA_ARGS__)
#define LOG_WARN(...) utils::log().warn(__VA_ARGS__)
#define LOG_ERROR(...) utils::log().error(__VA_ARGS__)
#define LOG_FATAL(...) utils::log().fatal(__VA_ARGS__)
#define LOG_SECTION(title) utils::log().section(title)
#define LOG_SEPARATOR() utils::log().separator()
#define LOG_FLUSH() utils::log().flush()