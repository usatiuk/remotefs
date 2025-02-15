//
// Created by stepus53 on 3.1.24.
//

#include "Options.h"

#include "Exception.h"

#include "Logger.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

Options& Options::get() {
    static Options opts;
    return opts;
}

void Options::reset() { get()._current = _defaults; }

void Options::reset(int argc, char* argv[]) {
    Options::reset();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg.length() < 2 || arg.substr(0, 2) != "--") {
            throw Exception("Can't parse argument " + arg);
        }
        std::string rest = arg.substr(2);

        std::vector<std::string> split;
        {
            std::istringstream ins(rest);
            std::string        cur;
            while (std::getline(ins, cur, ':')) {
                split.emplace_back(cur);
            }
        }

        if (split.empty())
            throw Exception("Can't parse argument " + arg);

        if (split.at(0) == "log") {
            if (split.size() != 3)
                throw Exception("Log options must be in format --log:TAG:LEVEL");
            try {
                Logger::set_level(Logger::str_to_tag(split.at(1)), std::stoi(split.at(2)));
            } catch (...) {
                throw Exception("Log options must be in format --log:TAG:LEVEL");
            }
        } else if (split.size() == 1) {
            std::string str = split.at(0);
            if (str.back() != '+' && str.back() != '-') {
                throw Exception("Bool options must be in format --option[+/-], instead have" + arg);
            }
            Options::set<bool>(str.substr(0, str.length() - 1), str.back() == '+' ? true : false);
        } else if (split.size() == 2) {
            try {
                Options::set<size_t>(split.at(0), std::stoul(split.at(1)));
            } catch (...) {
                Options::set<std::string>(split.at(0), split.at(1));
            }
        } else {
            throw Exception("Can't parse argument " + arg);
        }
    }
}
