cmake_minimum_required(VERSION 3.23)

project(LimitOrderBook)



# Library Target (LimitOrderBookLib)
add_library(LimitOrderBookLib)

target_sources(LimitOrderBookLib PRIVATE
    src/lib/order_core.cpp
    src/lib/order_book.cpp
    src/lib/matching_core.cpp
    src/lib/matching_engine.cpp
)

target_include_directories(LimitOrderBookLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
)

target_compile_features(LimitOrderBookLib PRIVATE cxx_std_20)

# Compiler Options
target_compile_options(LimitOrderBookLib PRIVATE
    $<$<CONFIG:Debug>: -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion>
    $<$<CONFIG:Release>: -O3 -march=native -flto -DNDEBUG>
)

# Macro Definitions
target_compile_definitions(LimitOrderBookLib PRIVATE
    $<$<CONFIG:Debug>:LOB_DEBUG>
    $<$<CONFIG:Release>:LOB_RELEASE>
)

# 
# Generator Executable
# 

add_executable(OrderEventGenerator)

target_sources(OrderEventGenerator PRIVATE
    src/order_event_generator/generator_main.cpp
)

# Link to LimitOrderBookLib
target_link_libraries(OrderEventGenerator PRIVATE LimitOrderBookLib)

target_compile_features(OrderEventGenerator PRIVATE cxx_std_20)

# 
# Main Executable
# 

add_executable(LimitOrderBookProgram)

target_sources(LimitOrderBookProgram PRIVATE
    src/main.cpp
)

# Link to LimitOrderBookLib
target_link_libraries(LimitOrderBookProgram PRIVATE LimitOrderBookLib)

target_compile_features(LimitOrderBookProgram PRIVATE cxx_std_20)

# 
# Testing
# 

enable_testing()
add_subdirectory(tests)
