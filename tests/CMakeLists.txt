# Testing with GoogleTest

# Fetch GoogleTest
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(RunTests
    order_core_test.cpp
    order_book_test.cpp
    matching_core_test.cpp
    matching_engine_test.cpp
)

# Link to LimitOrderBookLib and GoogleTest
target_link_libraries(RunTests PRIVATE
    LimitOrderBookLib
    gtest_main
)

target_compile_features(RunTests PRIVATE cxx_std_20)

add_test(NAME RunTests COMMAND RunTests)
