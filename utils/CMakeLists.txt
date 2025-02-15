cmake_minimum_required(VERSION 3.15)

add_library(utils
        include/Serialize.hpp
        src/Exception.cpp
        include/Exception.h
        include/stuff.hpp
        src/stuff.cpp
        include/Logger.h
        src/Logger.cpp
        src/Options.cpp
        include/Options.h
        include/SHA.h
        src/SHA.cpp
)

find_package(OpenSSL REQUIRED)
target_link_libraries(utils PRIVATE OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(utils PUBLIC include)

add_subdirectory(tests)