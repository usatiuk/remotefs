#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <netdb.h>
#include <unistd.h>

#include "FsClient.hpp"
#include "FsServer.hpp"
#include "Logger.h"
#include "Options.h"
#include "stuff.hpp"

int main(int argc, char* argv[]) {
    try {
        Options::reset(argc, argv);
        Logger::reset();

        if (Options::get<std::string>("mode") == "server") {
            FsServer().run();
        } else if (Options::get<std::string>("mode") == "client") {
            FsClient().run();
        } else {
            throw Exception("Unknown mode");
        }

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Crash!" << std::endl;
        return -1;
    }
    return 0;
}
