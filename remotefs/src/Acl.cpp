//
// Created by Stepan Usatiuk on 04.01.2025.
//

#include "Acl.hpp"

#include <sstream>
#include <unordered_set>

#include "Exception.h"
#include "Logger.h"
#include "SHA.h"
#include "stuff.hpp"

static const char int2hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

template<typename T>
std::string chars_to_hex(const T& v) {
    std::string out;
    out.resize(v.size() * 2);
    for (size_t i = 0; i < v.size(); i++) {
        out[2 * i]     = int2hex[(v[i] & 0xF0) >> 4];
        out[2 * i + 1] = int2hex[(v[i] & 0x0F)];
    }
    return out;
}

bool ACL::authorize(std::string username, std::string password) {
    auto hash = chars_to_hex(SHA::calculate(password));

    auto found_user = _users.find(username);

    if (found_user == _users.end()) {
        return false;
    }

    if (found_user->second != hash) {
        return false;
    }

    return true;
}

bool ACL::authorize_path(std::string username, std::string path) {
    // FIXME: could be faster with e.g. trie

    std::pair<std::string, std::unordered_set<std::string>> best_match{"", {}};

    for (const auto& filter: _filter) {
        if (path.starts_with(filter.first)) {
            if (filter.first.length() >= best_match.first.length()) {
                best_match = filter;
            }
        }
    }

    if (best_match.second.empty()) {
        return true;
    }

    if (best_match.second.contains(username)) {
        return true;
    }

    return false;
}

void ACL::load(std::string acl, std::string users) {
    auto users_lines = split(users, '\n');

    for (const auto& line: users_lines) {
        auto tokens = split(line, ' ');
        if (tokens.size() != 2) {
            throw Exception("Could not parse user definition: " + line);
        }

        if (tokens.at(1).length() != 64) {
            throw Exception("Could not parse user definition (wrong hash length): " + line);
        }

        _users.emplace(tokens.at(0), tokens.at(1));
    }

    auto acl_lines = split(acl, '\n');

    for (const auto& line: acl_lines) {
        auto tokens = split(line, ' ');
        if (tokens.empty())
            continue;

        std::unordered_set<std::string> usernames(tokens.begin() + 1, tokens.end());

        _filter.emplace(tokens.at(0), std::move(usernames));
    }

    Logger::log(
            Logger::RemoteFs,
            [&](std::ostream& os) {
                os << "Loaded users and ACL:\n";
                os << "Users:\n";
                for (const auto& user: _users) {
                    os << user.first << " " << user.second << '\n';
                }
                os << "ACL:\n";
                for (const auto& filter: _filter) {
                    os << filter.first << " ";
                    for (const auto& user: filter.second) {
                        os << " " << user;
                    }
                    os << '\n';
                }
            },
            Logger::INFO);
}
