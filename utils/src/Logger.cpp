//
// Created by stepus53 on 3.1.24.
//

#include "Logger.h"

#include <iomanip>
#include <sstream>

#include "Options.h"

Logger& Logger::get() {
    static Logger logger;
    return logger;
}

void Logger::log(LogTag tag, const std::string& what, int level) {
    if (!en_level(tag, level))
        return;
    {
        auto              now = std::chrono::high_resolution_clock::now();
        std::stringstream out;
        out << std::setprecision(3) << std::fixed << "["
            << static_cast<double>(
                       std::chrono::duration_cast<std::chrono::milliseconds>(now - get()._start_time).count()) /
                        1000.0
            << "s]"
            << "[" << tag_to_str(tag) << "][" << get()._level_names.at(level) << "] " << what << '\n';

        if (level == 1)
            get()._out_err.get() << out.str();
        else
            get()._out.get() << out.str();
    }
}

void Logger::log(LogTag tag, const std::function<void(std::ostream&)>& fn, int level) {
    if (!en_level(tag, level))
        return;

    std::stringstream out;
    fn(out);
    log(tag, out.str(), level);
}

void Logger::set_level(LogTag tag, int level) { get()._levels[tag] = static_cast<LogLevel>(level); }
void Logger::set_out(std::ostream& out) { get()._out = out; }
void Logger::set_out_err(std::ostream& out_err) { get()._out_err = out_err; }
void Logger::reset() { get()._levels.fill(static_cast<LogLevel>(Options::get<size_t>("default_log_level"))); }

int Logger::get_level(LogTag tag) { return get()._levels.at(tag); }

bool Logger::en_level(LogTag tag, int level) {
    int en_level = get_level(tag);
    if (en_level < level)
        return false;
    return true;
}

Logger::Logger() { _levels.fill(static_cast<LogLevel>(Options::get<size_t>("default_log_level"))); }
