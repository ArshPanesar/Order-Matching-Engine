#pragma once

// Branch Prediction Hints for the Compiler
#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY_BRANCH(condition) (__builtin_expect(!!(condition), 1))
    #define UNLIKELY_BRANCH(condition) (__builtin_expect(!!(condition), 0))
#else
    // No Hints for Other Compilers
    #define LIKELY_BRANCH(condition) (condition)
    #define UNLIKELY_BRANCH(condition) (condition)
#endif

