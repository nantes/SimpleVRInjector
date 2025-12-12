#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <windows.h>

#define LOG_INFO(msg)  std::cout << "[Info] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[Error] " << msg << std::endl

namespace Shared {
    struct Config {
        bool vrEnabled = true;
        float stereoSeparation = 0.05f;
    };
}
