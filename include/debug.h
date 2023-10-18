#pragma once
#include <iostream>

// To see advanced logging you need to: `#define LOG_ENABLED true` before any included file you want logging in. 
// Then you can set CurrentLevel to control while the application runs.

enum LogLevels {
    debug,
    info,
    warn,
    error,
    none
};

LogLevels LogLevel = none;

#ifdef LOG_ENABLED
#define LOG(level, message) \
if (level >= LogLevel) { \
    std::cout << std::string(__FILE__).substr(std::string(__FILE__).size() - 12) << " (" << __LINE__ << ") " << message << std::endl; \
}
#else
#define LOG(level, message)
#endif