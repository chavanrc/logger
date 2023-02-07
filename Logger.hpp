#pragma once
#undef SPDLOG_ACTIVE_LEVEL

#ifdef _LOG_LEVEL_TRACE_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#elif _LOG_LEVEL_DEBUG_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#elif _LOG_LEVEL_INFO_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#elif _LOG_LEVEL_WARN_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_WARN
#elif _LOG_LEVEL_ERROR_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#elif _LOG_LEVEL_CRITICAL_
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
#endif

#include "rapidyaml.hpp"
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/spdlog.h>

#include "Exceptions.hpp"

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define TRACELOG(...) SPDLOG_LEVEL_TRACE(findoc::Logger::logger_, __VA_ARGS__)
#else
#define TRACELOG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define DEBUGLOG(...) SPDLOG_LOGGER_DEBUG(findoc::Logger::logger_, __VA_ARGS__)
#else
#define DEBUGLOG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define INFOLOG(...) SPDLOG_LOGGER_INFO(findoc::Logger::logger_, __VA_ARGS__)
#else
#define INFOLOG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define WARNLOG(...) SPDLOG_LOGGER_WARN(findoc::Logger::logger_, __VA_ARGS__)
#else
#define WARNLOG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define ERRORLOG(...) SPDLOG_LOGGER_ERROR(findoc::Logger::logger_, __VA_ARGS__)
#else
#define ERRORLOG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define FATALLOG(...) SPDLOG_LOGGER_CRITICAL(findoc::Logger::logger_, __VA_ARGS__)
#else
#define FATALLOG(...) (void)0
#endif

namespace findoc {
    struct LoggerConfig {
        LoggerConfig() = default;

        /**
         * Constructs a LoggerConfig object from a YAML configuration.
         *
         * @param config The YAML configuration.
         *
         * @returns A LoggerConfig object.
         */
        explicit LoggerConfig(ryml::ConstNodeRef config) {
            getLoggerName(config);
            getPath(config);
            getLogLevel(config);
            getDevices(config);
            if (!validate()) {
                RAISE(ConfigException, "Invalid logger configuration.")
            }
            print();
        }

        /**
         * Gets the logger name from the environment variable or the config.
         *
         * @param config The config node.
         *
         * @returns None
         */
        void getLoggerName(ryml::ConstNodeRef config) {
            if (const char *env = std::getenv("LOGGER_NAME")) {
                loggerName_ = env;
                SPDLOG_INFO("env LOGGER_NAME={}", loggerName_);
            } else if (config.get_if("name", &loggerName_)) {
                SPDLOG_INFO("yaml name={}", loggerName_);
            } else {
                SPDLOG_DEBUG("Config name missing");
            }
            std::ranges::transform(loggerName_, loggerName_.begin(), ::toupper);
        }

        /**
         * Gets the path to the log file.
         *
         * @param config The configuration object.
         *
         * @returns None
         */
        void getPath(ryml::ConstNodeRef config) {
            if (const char *env = std::getenv("LOGGER_PATH")) {
                filePath_ = env;
                SPDLOG_INFO("env LOGGER_PATH={}", filePath_);
            } else if (config.get_if("path", &filePath_)) {
                SPDLOG_INFO("yaml path={}", filePath_);
            } else {
                SPDLOG_DEBUG("Config path missing");
            }
        }

        /**
         * Sets the log level.
         *
         * @param config The configuration node.
         *
         * @returns None
         */
        void getLogLevel(ryml::ConstNodeRef config) {
            std::string level;
            if (const char *env = std::getenv("LOGGER_LEVEL")) {
                level = env;
                SPDLOG_INFO("env LOGGER_LEVEL={}", level);
            } else if (config.get_if("level", &level)) {
                SPDLOG_INFO("yaml level={}", level);
            } else {
                SPDLOG_DEBUG("Config level missing");
            }
            if (level.empty()) {
                level = "info";
            }
            logLevel_ = spdlog::level::from_str(level);
        }

        /**
         * Gets the devices to use for logging.
         *
         * @param config The configuration object.
         *
         * @returns None
         */
        void getDevices(ryml::ConstNodeRef config) {
            if (const char *env = std::getenv("LOGGER_DEVICES")) {
                devices_.clear();
                devices_.emplace_back(env);
                SPDLOG_INFO("env LOGGER_DEVICES={}", devices_.back());
            } else if (config["devices"].is_seq()) {
                devices_.clear();
                config["devices"] >> devices_;
                SPDLOG_INFO("yaml devices={}", fmt::format("{}", fmt::join(devices_, ", ")));
            } else {
                SPDLOG_DEBUG("Config devices missing");
            }
            std::ranges::for_each(devices_, [](auto &device) {
                std::ranges::transform(device, device.begin(), ::tolower);
            });
        }

        /**
         * Initializes the logger.
         *
         * @param loggerName The name of the logger.
         * @param fileName The path to the log file.
         * @param level The log level.
         * @param hour The hour of the day to log at.
         * @param minute The minute of the hour to log at.
         * @param logPattern The log pattern.
         * @param queueSize The size of the log queue.
         * @param threadCount The number of threads to use.
         *
         * @returns None
         */
        void init(std::string loggerName, std::string fileName, const std::string &level,
                  int32_t hour = 23, int32_t minute = 59,
                  std::string logPattern = "[%Y-%m-%d %H:%M:%S.%f][%l][%s:%#] %v",
                  size_t queueSize = 8192, size_t threadCount = 1) {
            loggerName_ = std::move(loggerName);
            filePath_ = std::move(fileName);
            logLevel_ = spdlog::level::from_str(level);
            hour_ = hour;
            minute_ = minute;
            logPattern_ = std::move(logPattern);
            queueSize_ = queueSize;
            threadCount_ = threadCount;
        }

        /**
         * Validates the configuration of the logger.
         *
         * @returns True if the configuration is valid, false otherwise.
         */
        [[nodiscard]] bool validate() const {
            bool result = false;
            if (loggerName_.empty()) {
                SPDLOG_ERROR("logger name is empty");
                return result;
            }
            if (filePath_.empty()) {
                SPDLOG_ERROR("File name is empty");
                return result;
            }
            if (hour_ > 24 || hour_ < 0) {
                SPDLOG_ERROR("Invalid hour {} config", hour_);
                return result;
            }
            if (minute_ > 60 || minute_ < 0) {
                SPDLOG_ERROR("Invalid minute {} config", minute_);
                return result;
            }
            return !result;
        }

        /**
         * Prints the configuration of the logger.
         *
         * @returns None
         */
        void print() const {
            SPDLOG_INFO("Logger Config");
            auto devices = fmt::format("{}", fmt::join(devices_, ", "));
            SPDLOG_INFO(
                    "logger name: {}, file path: {}, hour: {}, minute: {}, log "
                    "pattern: {}, level: {}, queue size: {}, thread count: {}, devices: {}",
                    loggerName_, filePath_, hour_, minute_, logPattern_, logLevel_, queueSize_,
                    threadCount_, devices);
        }

        std::string loggerName_{"app"};
        std::string filePath_{"app.log"};
        int32_t hour_{23};
        int32_t minute_{59};
        std::string logPattern_{"[%Y-%m-%d %H:%M:%S.%f][%l][%s:%#] %v"};
        spdlog::level::level_enum logLevel_{spdlog::level::level_enum::info};
        size_t queueSize_{8192};
        size_t threadCount_{1};
        std::vector<std::string> devices_{"stdout"};
    };

    /**
     * Logger class that provides a singleton instance of a spdlog logger.
     *
     * @returns The singleton instance of the logger.
     */
    class Logger {
    public:
        /**
         * Initializes the logger.
         *
         * @param config The configuration for the logger.
         *
         * @returns None
         */
        static void init(ryml::ConstNodeRef config = ryml::parse_in_arena(R"(
                logger:
                    name: "tests"
                    devices: ["file", "stdout"]
                    path: "tests.log"
                    level: "debug"
                )")["logger"]) {
            findoc::LoggerConfig loggerConfig(config);
            init(loggerConfig);
        }

        /**
         * Initializes the logger.
         *
         * @param config The configuration for the logger.
         *
         * @returns None
         */
        static void init(const LoggerConfig &config) {
            static std::once_flag flag;
            std::call_once(flag, [&config]() {
                spdlog::init_thread_pool(config.queueSize_, config.threadCount_);
                auto sinks = getSinks(config);
                logger_ = std::make_shared<spdlog::async_logger>(
                        config.loggerName_, begin(sinks), end(sinks), spdlog::thread_pool(),
                        spdlog::async_overflow_policy::overrun_oldest);
                logger_->set_pattern(config.logPattern_);
                logger_->set_level(config.logLevel_);
                logger_->flush_on(spdlog::level::info);
                spdlog::register_logger(logger_);
                spdlog::set_default_logger(logger_);
            });
        }

        /**
         * Flushes the log.
         *
         * @returns None
         */
        static void flush() { logger_->flush(); }

        inline static std::shared_ptr<spdlog::async_logger> logger_{nullptr};

    private:
        static std::vector<spdlog::sink_ptr> getSinks(const LoggerConfig &config) {
            std::vector<spdlog::sink_ptr> sinks;
            if (std::ranges::find(config.devices_, "stdout") != config.devices_.end()) {
                sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
            }
            if (std::ranges::find(config.devices_, "stderr") != config.devices_.end()) {
                sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>());
            }
            if (std::ranges::find(config.devices_, "file") != config.devices_.end()) {
                sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(
                        config.filePath_, config.hour_, config.minute_));
            }
            if (sinks.empty()) {
                sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
            }
            return sinks;
        }
    };
}  // namespace findoc