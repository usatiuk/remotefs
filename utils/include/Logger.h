//
// Created by stepus53 on 3.1.24.
//

#ifndef PSIL_LOGGER_H
#define PSIL_LOGGER_H

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class Logger {
public:
    Logger();

    enum LogLevel { ALWAYS = 0, ERROR = 1, INFO = 2, DEBUG = 3, TRACE = 4 };

    enum LogTag { RemoteFs, LogTagMax };

    static void log(LogTag tag, const std::string& what, int level = INFO);
    static void log(LogTag tag, const std::function<void(std::ostream&)>& fn, int level = INFO);

    // 0 - disabled
    // 1 - error
    // 2 - info
    // 3 - debug
    // 4 - trace
    static void set_level(LogTag tag, int level);
    static int  get_level(LogTag tag);
    static bool en_level(LogTag tag, int level);

    static void set_out(std::ostream& out);
    static void set_out_err(std::ostream& out_err);

    static void reset();

    static Logger& get();

    static LogTag             str_to_tag(const std::string& str) { return _str_to_tag.at(str); }
    static const std::string& tag_to_str(LogTag tag) { return _tag_to_str.at(tag); }

private:
    std::array<LogLevel, LogTag::LogTagMax>            _levels{};
    static inline std::unordered_map<int, std::string> _level_names{
            {ALWAYS, "ALWAYS"}, {ERROR, "ERROR"}, {INFO, "INFO"}, {DEBUG, "DEBUG"}, {TRACE, "TRACE"},
    };
    static inline std::unordered_map<std::string, LogTag> _str_to_tag{
            {"Server", RemoteFs},
    };
    static inline std::unordered_map<LogTag, std::string> _tag_to_str{
            {RemoteFs, "Server"},
    };

    std::chrono::time_point<std::chrono::high_resolution_clock> _start_time = std::chrono::high_resolution_clock::now();
    std::reference_wrapper<std::ostream>                        _out        = std::cout;
    std::reference_wrapper<std::ostream>                        _out_err    = std::cerr;
};


#endif // PSIL_LOGGER_H
