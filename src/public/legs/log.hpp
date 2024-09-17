#pragma once

#include <cstdio>
#include <cstring>
#include <format>
#include <iostream>
#include <mutex>
#include <ostream>

#include <legs/time.hpp>

namespace legs
{
enum LogLevel
{
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    MAX,
};

class Log
{
  public:
    static void SetLogLevel(const LogLevel level)
    {
        m_logLevel = level;
    }

    template<typename... Args>
    static void Print(
        const char*        file,
        const unsigned int line,
        const char*        func,
        const LogLevel     level,
        const char*        fmt,
        Args&&... args
    )
    {
        if (level < m_logLevel)
        {
            return;
        }

        auto vargs = std::vformat(fmt, std::make_format_args(args...));

        auto message = std::format(
            "[{:.3f}]"      // Time
            "[{}]"          // Severity
            "[{}:{}@{}()] " // Location
            "{}",           // Message
            Time::Now(),
            m_severityStrings[static_cast<int>(level)],
            file,
            line,
            func,
            vargs
        );

        const std::scoped_lock lock {m_logMutex};

        std::cout << message << '\n';

#if !NDEBUG
        Flush();
#endif
    }

    static void Flush()
    {
        std::flush(std::cout);
    }

  private:
    static inline LogLevel m_logLevel;

    static constexpr const char* m_severityStrings[static_cast<int>(LogLevel::MAX)] = {
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
    };

    static inline std::mutex m_logMutex;
};

#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)

#define _LOG(L, F, ...) legs::Log::Print(__FILENAME__, __LINE__, __func__, L, F, ##__VA_ARGS__)

#define LOG_DEBUG(F, ...) _LOG(legs::LogLevel::Debug, F, ##__VA_ARGS__)
#define LOG_INFO(F, ...)  _LOG(legs::LogLevel::Info, F, ##__VA_ARGS__)
#define LOG_WARN(F, ...)  _LOG(legs::LogLevel::Warn, F, ##__VA_ARGS__)
#define LOG_ERROR(F, ...) _LOG(legs::LogLevel::Error, F, ##__VA_ARGS__)
#define LOG_FATAL(F, ...) _LOG(legs::LogLevel::Fatal, F, ##__VA_ARGS__)

} // namespace legs
