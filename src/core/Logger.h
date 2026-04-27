// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file Logger.h
 * @brief Minimal structured logging with compile-time and runtime level control.
 *
 * Log levels: DEBUG, INFO, WARN, ERROR.
 *
 * Compile-time control:
 *   #define QE_LOG_LEVEL_MIN QE_LOG_WARN   // before including this header
 *   Levels below the minimum are compiled out entirely (zero overhead).
 *
 * Runtime control:
 *   qe::core::Logger::set_level(qe::core::LogLevel::Warn);
 *
 * Usage:
 *   QE_LOG_INFO("Gamepad")  << "Connected: " << name << std::endl;
 *   QE_LOG_ERROR("Shader")  << "Compile error: " << log << std::endl;
 *   QE_LOG_DEBUG("Physics") << "dt=" << dt << std::endl;
 */

#ifndef QE_CORE_LOGGER_H
#define QE_CORE_LOGGER_H

#include <iostream>
#include <streambuf>

namespace qe {
namespace core {

enum class LogLevel : int {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Off   = 4
};

// Numeric constants for compile-time filtering.
#define QE_LOG_DEBUG_LEVEL 0
#define QE_LOG_INFO_LEVEL  1
#define QE_LOG_WARN_LEVEL  2
#define QE_LOG_ERROR_LEVEL 3
#define QE_LOG_OFF_LEVEL   4

#ifndef QE_LOG_LEVEL_MIN
#define QE_LOG_LEVEL_MIN QE_LOG_DEBUG_LEVEL
#endif

class NullBuffer : public std::streambuf {
protected:
    int overflow(int ch) override { return ch; }
};

/** A no-op stream that discards everything written to it. */
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&buffer_) {}

private:
    NullBuffer buffer_;
};

class Logger {
public:
    /** Set the runtime minimum log level. */
    static void set_level(LogLevel level) { min_level() = level; }

    /** Get the current runtime minimum log level. */
    static LogLevel level() { return min_level(); }

    /** Check if a given level is enabled at runtime. */
    static bool enabled(LogLevel level) {
        return static_cast<int>(level) >= static_cast<int>(min_level());
    }

private:
    static LogLevel& min_level() {
        static LogLevel lvl = LogLevel::Info;
        return lvl;
    }

    // Allow macros to access internals.
    template <LogLevel L> friend struct LogHelper;
};

// ── Logging Macros ──────────────────────────────────────────────────────────

// Internal: returns the correct stream with a prefix, or a null stream.
#define QE_LOG_IMPL(level_enum, level_int, level_tag, category)                \
    ((QE_LOG_LEVEL_MIN <= (level_int) &&                                        \
      qe::core::Logger::enabled(qe::core::LogLevel::level_enum))               \
         ? static_cast<std::ostream&>(                                          \
               ((qe::core::LogLevel::level_enum >= qe::core::LogLevel::Warn)    \
                    ? std::cerr                                                 \
                    : std::cout)                                                \
               << "[" level_tag "] [" category "] ")                           \
         : qe::core::detail::null_stream())

namespace detail {
    inline std::ostream& null_stream() {
        static NullStream ns;
        return ns;
    }
} // namespace detail

} // namespace core
} // namespace qe

// Public macros: QE_LOG_DEBUG("tag") << "message" << std::endl;
#define QE_LOG_DEBUG(cat) QE_LOG_IMPL(Debug, QE_LOG_DEBUG_LEVEL, "DEBUG", cat)
#define QE_LOG_INFO(cat)  QE_LOG_IMPL(Info,  QE_LOG_INFO_LEVEL,  "INFO",  cat)
#define QE_LOG_WARN(cat)  QE_LOG_IMPL(Warn,  QE_LOG_WARN_LEVEL,  "WARN",  cat)
#define QE_LOG_ERROR(cat) QE_LOG_IMPL(Error, QE_LOG_ERROR_LEVEL, "ERROR", cat)

#endif // QE_CORE_LOGGER_H
