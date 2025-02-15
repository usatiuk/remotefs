cmake_minimum_required(VERSION 3.15)

add_library(networking
        include/Server.hpp
        src/Server.cpp
        include/Client.hpp
        src/Client.cpp
        include/Helpers.hpp
        src/Helpers.cpp
        src/AsyncSslTransport.cpp
        include/AsyncSslTransport.hpp
        src/AsyncSslClientTransport.cpp
        include/AsyncSslClientTransport.hpp
        include/AsyncSslServerTransport.hpp
        src/AsyncSslServerTransport.cpp
)

target_include_directories(networking PUBLIC include)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED IMPORTED_TARGET fuse)

target_link_libraries(networking PRIVATE utils OpenSSL::SSL OpenSSL::Crypto PkgConfig::FUSE)
