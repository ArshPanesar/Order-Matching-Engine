#pragma once
#include <cassert>
#include <iostream>

// Debug Assertion
#ifdef LOB_DEBUG
    #define LOB_ASSERT(expr) assert(expr) 
#else
    #define LOB_ASSERT(expr) ((void)0)
#endif

// Terminates the Program due to given Error
template<typename... Args>
[[noreturn]] inline void TerminateOnError(Args&&... args) {
    std::cerr << "[TERMINATION ERROR] - ";
    (std::cerr << ... << args);
    std::cerr << '\n';
    std::abort();
}

#define TERMINATE_ON_ERROR(...) TerminateOnError("In ", __FILE__, ":", __LINE__, " -> ", __VA_ARGS__)
