//
// Created by stepus53 on 3.1.24.
//

#ifndef PSIL_OPTIONS_H
#define PSIL_OPTIONS_H


#include <cstddef>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include "Exception.h"

class Options {
public:
    template<typename T>
    static T get(const std::string& opt) {
        Options& o = get();
        if (_defaults.find(opt) == _defaults.end())
            throw Exception("Unknown option " + opt);
        if (!std::holds_alternative<T>(_defaults.at(opt)))
            throw Exception("Bad option type " + opt);
        return std::get<T>(o._current.at(opt));
    }

    template<typename T>
    static void set(const std::string& opt, const T& val) {
        Options& o = get();
        if (_defaults.find(opt) == _defaults.end())
            throw Exception("Unknown option " + opt);
        if (!std::holds_alternative<T>(_defaults.at(opt)))
            throw Exception("Bad option type " + opt);
        o._current[opt] = val;
    }

    static void     reset();
    static void     reset(int argc, char* argv[]);
    static Options& get();

private:
    using OptionType = std::variant<size_t, std::string, bool>;

    const static inline std::unordered_map<std::string, OptionType> _defaults{{"ip", "127.0.0.1"},
                                                                              {"port", 42069U},
                                                                              {"default_log_level", 2U},
                                                                              {"timeout", 30U},
                                                                              {"ca_path", "cert.pem"},
                                                                              {"pk_path", "key.pem"},
                                                                              {"mode", "server"},
                                                                              {"path", ""},
                                                                              {"acl_path", ""},
                                                                              {"users_path", ""},
                                                                              {"username", ""},
                                                                              {"password", ""}};

    std::unordered_map<std::string, OptionType> _current = _defaults;
};


#endif // PSIL_OPTIONS_H
