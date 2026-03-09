# Benchmarks with std::chrono

add_executable(Benchmarks
    benchmarks.cpp
    ../src/generator/order_event_generator.cpp
)

# Link to LimitOrderBookLib
target_link_libraries(Benchmarks PRIVATE
    LimitOrderBookLib
)

target_compile_features(Benchmarks PRIVATE cxx_std_20)

target_compile_options(Benchmarks PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -flto -DNDEBUG>
)