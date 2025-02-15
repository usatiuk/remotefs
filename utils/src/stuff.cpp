//
// Created by Stepan Usatiuk on 04.01.2025.
//


#include "stuff.hpp"

#include <sstream>

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::string              token;

    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delim))
        elems.push_back(token);

    return elems;
}
