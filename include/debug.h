#pragma once
#include <iostream>

// To see logging you need to: `#define LOG_ENABLED true` before any included file you want logging in. 
// Then you can set LogLevel to control while the application runs.

// To have validation logic run you need to: `#define VALIDATION_ENABLED true` before any included file you want validation in. 
// Then you can set ValidationLevel to control while the application runs.

// To have assert logic run you need to: `#define ASSERT_ENABLED true` before any included file you want validation in. 
// Then you can set AssertLevel to control while the application runs.

int DebugFilenameMaxLength = 12;

#define ECHO(message) std::cout << std::string(__FILE__).substr(std::string(__FILE__).size() - DebugFilenameMaxLength) << " (" << __LINE__ << ") " << message << std::endl;

enum Levels {
    debug,
    info,
    warn,
    error,
    none
};

Levels LogLevel = none;

#ifdef LOG_ENABLED
#define LOG(level, message) \
if (level >= LogLevel) { \
    ECHO(message) \
}
#else
#define LOG(level, message)
#endif

Levels ValidationLevel = none;

#ifdef VALIDATION_ENABLED
#define VALIDATE(level, body) \
if (level >= ValidationLevel) body
#else
#define VALIDATE(level, body)
#endif

Levels AssertLevel = none;

#ifdef ASSERT_ENABLED
#define ASSERT(level, condition, message) \
if (level >= AssertLevel && !(condition)) { \
    ECHO(message) \
}
#define ASSERT_THROW(level, condition, message) \
if (level >= AssertLevel && !(condition)) { \
    throw std::invalid_argument(message); \
}
#else
#define ASSERT(level, condition, message)
#define ASSERT_THROW(level, condition, message)
#endif