//
// Created by Stepan Usatiuk on 01.05.2023.
//

#include "Exception.h"

#include "stuff.hpp"

#include <execinfo.h>
#include <sstream>

#include <openssl/err.h>

Exception::Exception(const std::string& text) : runtime_error(text + "\n" + getStacktrace()) {}

Exception::Exception(const char* text) : runtime_error(std::string(text) + "\n" + getStacktrace()) {}

// Based on: https://www.gnu.org/software/libc/manual/html_node/Backtraces.html
std::string Exception::getStacktrace() {
    std::vector<void*> functions(50);
    char**             strings;
    int                n;

    n       = backtrace(functions.data(), 50);
    strings = backtrace_symbols(functions.data(), n);

    std::stringstream out;

    if (strings != nullptr) {
        out << "Stacktrace:" << std::endl;
        for (int i = 0; i < n; i++)
            out << strings[i] << std::endl;
    }

    free(strings);
    return out.str();
}

// https://stackoverflow.com/questions/42106339/how-to-get-the-error-string-in-openssl
static std::string OpenSSLErrorToString() {
    BIO* bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);
    char*       buf;
    size_t      len = checked_cast<size_t>(BIO_get_mem_data(bio, &buf));
    std::string ret(buf, len);
    BIO_free(bio);
    return ret;
}

OpenSSLException::OpenSSLException(const std::string& text) : Exception(text + "\n" + OpenSSLErrorToString()) {}
OpenSSLException::OpenSSLException(const char* text) : Exception(std::string(text) + "\n" + OpenSSLErrorToString()) {}
