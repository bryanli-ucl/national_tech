#include "logger.hpp"

// Platform-specific includes and macros
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define ACCESS _access
#define MKDIR(path) _mkdir(path)
#define GETCWD _getcwd
#else
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS access
#define MKDIR(path) mkdir(path, 0755)
#define GETCWD getcwd
#endif

namespace utils {

/**
 * @brief Constructor - Initialize logger with default settings
 * Starts the background worker thread for asynchronous logging
 */
Logger::Logger()
: m_min_level(LogLevel::INFO),
  m_show_timestamp(true),
  m_show_level(true),
  m_colorize(true),
  m_running(true),
  m_worker_finished(false),
  m_log_to_file(false) {
    // Start worker thread
    m_worker_thread = std::thread(&Logger::process_logs, this);
}

/**
 * @brief Destructor - Clean shutdown of logger
 * Stops the worker thread and closes log file
 */
Logger::~Logger() {
    // Signal worker thread to stop
    m_running.store(false, std::memory_order_release);
    m_cv.notify_one();

    // Wait for worker thread to finish
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }

    // Close log file if open
    std::lock_guard<std::mutex> lock(m_file_mutex);
    if (m_file_stream.is_open()) {
        m_file_stream.flush();
        m_file_stream.close();
    }
}

/**
 * @brief Worker thread function to process log messages
 * Continuously processes messages from queue and outputs to console and file
 */
auto Logger::process_logs() -> void {
    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);

        // Wait for messages or stop signal
        m_cv.wait(lock, [this] {
            return !m_log_queue.empty() || !m_running.load(std::memory_order_acquire);
        });

        // Process all pending log messages
        while (!m_log_queue.empty()) {
            std::string log_msg = std::move(m_log_queue.front());
            m_log_queue.pop();
            lock.unlock();

            // Output to console
            std::cout << log_msg << std::endl;

            // Output to file if enabled
            if (m_log_to_file.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> file_lock(m_file_mutex);
                if (m_file_stream.is_open()) {
                    m_file_stream << removeColorCodes(log_msg) << std::endl;
                    m_file_stream.flush(); // Flush after each log entry
                }
            }

            lock.lock();
        }

        // Exit loop if stopped and queue is empty
        if (!m_running.load(std::memory_order_acquire) && m_log_queue.empty()) {
            break;
        }
    }

    // Mark worker as finished
    m_worker_finished.store(true, std::memory_order_release);
}

/**
 * @brief Remove ANSI color codes from string
 * Strips escape sequences for clean file output
 * @param str Input string potentially containing ANSI codes
 * @return String without color codes
 */
auto Logger::removeColorCodes(const std::string& str) -> std::string {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        // Check for ANSI escape sequence start
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') {
            // Skip until 'm' character (end of ANSI code)
            size_t j = i + 2;
            while (j < str.size() && str[j] != 'm') {
                ++j;
            }
            i = j; // Skip the entire escape sequence
        } else {
            result += str[i];
        }
    }

    return result;
}

/**
 * @brief Format current timestamp with millisecond precision
 * Uses caching to avoid redundant formatting for the same second
 * @return Formatted timestamp string [HH:MM:SS.mmm]
 */
auto Logger::formatTimestamp() -> std::string {
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
              now.time_since_epoch()) %
    1000;

    auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch())
                       .count();

    int64_t cached_second = m_time_cache.last_second.load(std::memory_order_relaxed);

    std::string time_str;

    // Check if we need to update the cached time string
    if (cached_second != now_seconds) {
        std::lock_guard<std::mutex> lock(m_time_cache.mutex);

        // Double-check after acquiring lock
        cached_second = m_time_cache.last_second.load(std::memory_order_relaxed);
        if (cached_second != now_seconds) {
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;

            // Platform-specific localtime conversion
#ifdef _WIN32
            localtime_s(&tm, &time_t);
#else
            localtime_r(&time_t, &tm);
#endif

            // Format time string (HH:MM:SS)
            std::ostringstream oss;
            oss << std::put_time(&tm, "%H:%M:%S");
            m_time_cache.cached_time_str = oss.str();
            m_time_cache.last_second.store(now_seconds, std::memory_order_relaxed);
        }
        time_str = m_time_cache.cached_time_str;
    } else {
        // Use cached time string
        std::lock_guard<std::mutex> lock(m_time_cache.mutex);
        time_str = m_time_cache.cached_time_str;
    }

    // Build final timestamp with milliseconds
    std::ostringstream result;
    if (m_colorize) result << GRAY;
    result << "[" << time_str << '.';
    result << std::setfill('0') << std::setw(3) << ms.count() << "]";
    if (m_colorize) result << RESET;

    return result.str();
}

/**
 * @brief Generate log filename based on current timestamp
 * Format: logs/YYYY-MM-DD_HH-MM-SS.log
 * @return Generated log filename
 */
auto Logger::generateLogFilename() -> std::string {
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;

    // Platform-specific localtime conversion
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    // Format filename with timestamp
    std::ostringstream oss;
    oss << "logs/";
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    oss << ".log";

    return oss.str();
}

/**
 * @brief Create directory if it doesn't exist
 * @param path Directory path to create
 * @return true if directory exists or was created successfully
 */
auto Logger::createDirectory(const std::string& path) -> bool {
    if (path.empty()) return false;

    // Check if directory already exists
    if (ACCESS(path.c_str(), 0) == 0) {
        return true;
    }

    // Attempt to create directory
    int result = MKDIR(path.c_str());
    if (result == 0) {
        return true;
    }

    // Double-check in case another thread created it
    return ACCESS(path.c_str(), 0) == 0;
}

/**
 * @brief Get singleton instance of Logger
 * @return Reference to the Logger instance
 */
auto Logger::instance() -> Logger& {
    static Logger instance;
    return instance;
}

/**
 * @brief Enable file logging
 * Creates log directory if needed and opens a new log file
 * @return true if file logging was enabled successfully
 */
auto Logger::enableFileLogging() -> bool {
    std::lock_guard<std::mutex> file_lock(m_file_mutex);

    // Close existing log file if open
    if (m_file_stream.is_open()) {
        m_file_stream.flush();
        m_file_stream.close();
    }

    // Create logs directory
    if (!createDirectory("logs")) {
        std::cerr << "ERROR: Failed to create logs directory" << std::endl;
        m_log_to_file.store(false, std::memory_order_release);
        return false;
    }

    // Generate log filename
    m_log_file_path = generateLogFilename();

    // Open log file
    m_file_stream.open(m_log_file_path, std::ios::out | std::ios::trunc);

    if (!m_file_stream.is_open()) {
        std::cerr << "ERROR: Failed to open log file: " << m_log_file_path << std::endl;
        std::cerr << "ERROR: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
        m_log_to_file.store(false, std::memory_order_release);
        return false;
    }

    // Write file header
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;

#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    m_file_stream << "========== Log Session Started: ";
    m_file_stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    m_file_stream << " ==========\n"
                  << std::endl;
    m_file_stream.flush();

    // Enable file logging flag
    m_log_to_file.store(true, std::memory_order_release);

    std::cout << "Log file created: " << m_log_file_path << std::endl;
    return true;
}

/**
 * @brief Disable file logging
 * Closes the log file and disables file output
 */
auto Logger::disableFileLogging() -> void {
    // Disable file logging flag
    m_log_to_file.store(false, std::memory_order_release);

    // Close log file
    std::lock_guard<std::mutex> lock(m_file_mutex);
    if (m_file_stream.is_open()) {
        m_file_stream.flush();
        m_file_stream.close();
    }
}

/**
 * @brief Get current log file path
 * @return Path to current log file
 */
auto Logger::getLogFilePath() const -> std::string {
    return m_log_file_path;
}

/**
 * @brief Flush all pending log messages
 * Waits for queue to be processed and flushes file stream
 */
auto Logger::flush() -> void {
    // Wait for queue to be processed (max 500ms)
    for (int i = 0; i < 50; ++i) {
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_log_queue.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Flush file stream
    std::lock_guard<std::mutex> lock(m_file_mutex);
    if (m_file_stream.is_open()) {
        m_file_stream.flush();
    }
}

/**
 * @brief Output a separator line
 * @param c Character to use for separator
 * @param length Length of separator line
 */
auto Logger::separator(char c, int length) -> void {
    enqueueLog(std::string(length, c));
}

/**
 * @brief Output a section header with separator lines
 * @param title Section title text
 */
auto Logger::section(const std::string& title) -> void {
    enqueueLog("\n");
    separator('=');
    enqueueLog("  " + title);
    separator('=');
}

/**
 * @brief Enqueue log message to processing queue
 * Thread-safe operation that adds message to queue and notifies worker
 * @param log_msg Log message to enqueue
 */
auto Logger::enqueueLog(const std::string& log_msg) -> void {
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_log_queue.push(log_msg);
    }
    // Notify worker thread that a message is available
    m_cv.notify_one();
}

} // namespace utils
