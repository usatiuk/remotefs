//
// Created by stepus53 on 24.8.24.
//

#ifndef STUFF_H
#define STUFF_H

#include <cassert>
#include "Exception.h"


#ifdef __APPLE__
#include <machine/endian.h>
#define htobe64(x) htonll(x)
#define be64toh(x) ntohll(x)
#else
#include <endian.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"

template<typename To, typename From>
constexpr To checked_cast(const From& f) {
    To result = static_cast<To>(f);
    if (f != result) {
        throw Exception("Value out of bounds");
    }
    return result;
}

#pragma GCC diagnostic pop

std::vector<std::string> split(const std::string& s, char delim);

#endif // STUFF_H
