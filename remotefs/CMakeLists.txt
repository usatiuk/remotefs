cmake_minimum_required(VERSION 3.15)

add_library(remotefs_lib
        include/FsClient.hpp
        src/FsClient.cpp
        include/FsServer.hpp
        src/FsServer.cpp
        include/Messages.hpp
        src/Acl.cpp
        include/Acl.hpp
)

target_include_directories(remotefs_lib PUBLIC include)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED IMPORTED_TARGET fuse)

target_link_libraries(remotefs_lib PUBLIC
        utils
        networking
        OpenSSL::SSL
        OpenSSL::Crypto
        PkgConfig::FUSE
)

add_executable(remotefs src/main.cpp)

target_link_libraries(remotefs PRIVATE remotefs_lib)

add_subdirectory(tests)
