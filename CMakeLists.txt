cmake_minimum_required(VERSION 3.23)

project(LimitOrderBook)



# Library Target (LimitOrderBookLib)
add_library(LimitOrderBookLib)

target_sources(LimitOrderBookLib PRIVATE
    src/lib/memory/memory_core.cpp
    src/lib/order_core.cpp
    src/lib/order_book_core.cpp
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
    $<$<CONFIG:RelWithDebInfo>: -O3 -g -fno-omit-frame-pointer>
    $<$<CONFIG:Release>: -O3 -march=native -DNDEBUG>
)

# Enable LTO for Release Builds
set_target_properties(LimitOrderBookLib PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# Macro Definitions
target_compile_definitions(LimitOrderBookLib PRIVATE
    $<$<CONFIG:Debug>:LOB_DEBUG>
    $<$<CONFIG:Release>:LOB_RELEASE>
)

# 
# Tools Executable
# 
add_executable(Tools)

target_sources(Tools PRIVATE
    src/generator/order_event_generator.cpp
    src/util/spsc_queue.cpp
    src/tools.cpp
)

# Compiler Options
target_compile_options(Tools PRIVATE
    $<$<CONFIG:Debug>: -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion>
    $<$<CONFIG:RelWithDebInfo>: -O3 -g -fno-omit-frame-pointer>
    $<$<CONFIG:Release>: -O3 -march=native -DNDEBUG>
)

# Link to LimitOrderBookLib
target_link_libraries(Tools PRIVATE LimitOrderBookLib)

target_compile_features(Tools PRIVATE cxx_std_20)

# Enable LTO for Release Builds
set_target_properties(Tools PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

target_compile_definitions(Tools PRIVATE
    $<$<CONFIG:Debug>:LOB_DEBUG>
    $<$<CONFIG:Release>:LOB_RELEASE>
)

# 
# Main Executable
# 

add_executable(OrderMatchingEngine)

target_sources(OrderMatchingEngine PRIVATE
    src/generator/order_event_generator.cpp
    src/util/spsc_queue.cpp
    src/main.cpp
)

# Compiler Options
target_compile_options(OrderMatchingEngine PRIVATE
    $<$<CONFIG:Debug>: -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion>
    $<$<CONFIG:RelWithDebInfo>: -O3 -g -fno-omit-frame-pointer>
    $<$<CONFIG:Release>: -O3 -march=native -DNDEBUG>
)

# Link to LimitOrderBookLib
target_link_libraries(OrderMatchingEngine PRIVATE LimitOrderBookLib)

target_compile_features(OrderMatchingEngine PRIVATE cxx_std_20)

# Enable LTO for Release Builds
set_target_properties(OrderMatchingEngine PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

target_compile_definitions(OrderMatchingEngine PRIVATE
    $<$<CONFIG:Debug>:LOB_DEBUG>
    $<$<CONFIG:Release>:LOB_RELEASE>
)


# 
# Testing (Only for Debug Builds)
# 
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_testing()
    add_subdirectory(tests)
endif()


#
# Benchmarks (Only for Release Builds)
#
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_subdirectory(benchmarks)
endif()