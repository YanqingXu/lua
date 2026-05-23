#pragma once

#if defined(_MSC_VER)
#define LUA_DIAGNOSTIC_PUSH __pragma(warning(push))
#define LUA_DIAGNOSTIC_POP __pragma(warning(pop))
#define LUA_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS __pragma(warning(disable : 4996))
#elif defined(__clang__)
#define LUA_DIAGNOSTIC_PUSH _Pragma("clang diagnostic push")
#define LUA_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
#define LUA_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__)
#define LUA_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#define LUA_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#define LUA_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#else
#define LUA_DIAGNOSTIC_PUSH
#define LUA_DIAGNOSTIC_POP
#define LUA_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS
#endif

#define LUA_SUPPRESS_DEPRECATED_DECLARATIONS_BEGIN \
    LUA_DIAGNOSTIC_PUSH                              \
    LUA_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS

#define LUA_SUPPRESS_DEPRECATED_DECLARATIONS_END LUA_DIAGNOSTIC_POP
