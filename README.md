
[![CI](https://github.com/ArshPanesar/Order-Matching-Engine/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/ArshPanesar/Order-Matching-Engine/actions/workflows/ci.yml)

# Order Matching Engine

A High-Performance Limit Order Book implementation (written in C++20).

## Components

### Matching Engine Library (LimitOrderBookLib)

This library contains the Matching Engine and Order Book components. The implementation is optimized for instruments with a bounded range of prices. 

### Order Matching Engine Executable

This is a CLI application that runs the Matching Engine on a given Input binary file that stores a given number of Order Events (see below on generating a Binary file containing randomized Order Events). Any executed trades are represented via Trade Events and are also stored in a Binary File.

The application also uses a configuration file required to setup the Matching Engine before running it. 

#### Usage

```bash
./OrderMatchingEngine   --input <input_file_path> 
                        --output <output_file_path> 
                        --config <config_file_path>
```

#### Configuration File Format

Configuration fields are written as `key=value`. There are exactly 4 fields you can configure (all must fit an `unsigned long` type integer):
- `max_order_events` - Maximum Number of Order Events the Matching Engine should be setup for.
- `max_live_orders` - Maximum Number of Orders that may rest in the Order Book.
- `min_price` - Minimum Price of the Symbol/Instrument.  
- `max_price` - Maximum Price of the Symbol/Instrument.

The `min_price` and `max_price` fields represent ranges in `unsigned integers`.

A `default.conf` configuration file is provided in the repo.


## Tools Executable

This CLI Tool exists for the following purposes:
- Generating Order Events via a generator built to represent realistic market flow.
- Formatting a generated Order Events file from binary to a readable text format.
- Formatting a generated Trade Events file from binary to a readable text format.

### Usage

**Generating Events**

```bash
./Tools generate_events --output <output_file_path> 
                        --config <config_file_path>
```
- `output_file_path` must end with `.bin`. For example, `events.bin`.
- NOTE: The Order Event Generator internally uses a Matching Engine to generate a more realistic flow of events, therefore a configuration file is required. It is necessary that the same config file (or a config file with larger bounds) is used when running the OrderMatchingEngine executable on this input.


**Formatting Order Events Binary to Readable Text**

```bash
./Tools format_events   --input <input_binary_file_path> 
                        --output <output_text_file_path> 
```

**Formatting Trade Events Binary to Readable Text**

```bash
./Tools format_trades   --input <input_binary_file_path> 
                        --output <output_text_file_path> 
```

**Additional Options for** `generate_events`

- `--gen-seed` - RNG Seed for the `OrderEventGenerator`.
- `--gen-limit-prob` - Probability of an Order Event being a Limit Order.
- `--gen-bid-prob` - Probability of an Order Event being a Buy/Bid side Order
- `--gen-event-prob-array` - 3 Probabilities (Must Sum to 1) of an OrderEvent being of New, Cancel, or Amend Type. Format: `--gen-event-prob-array p1,p2,p3`.

## Project Structure
```
.
├── benchmarks/                  # Benchmarking code and results
│   ├── benchmarks.cpp           # C++ benchmarks
│   ├── CMakeLists.txt
│   ├── pipeline_benchmark.py    # Python pipeline test
│
├── src/
│   ├── lib/                     # Core library
│   │   ├── matching_engine.*    # Main matching logic
│   │   ├── order_book.*         # Order book interface
│   │   ├── order_book_core.*    # Order book data structures
│   │   ├── order_core.*         # Order defs
│   │   ├── matching_core.*      # OrderEvent/TradeEvent defs
│   │   ├── memory/              # Custom allocator
│   │   └── util/                # Helper utilities
│   │
│   ├── generator/               # Order event generator
│   ├── util/                    # Utility functions
│   ├── main.cpp                 # Matching engine executable
│   └── tools.cpp                # Tools executable
│
├── tests/                       # Unit tests (68 tests)
│   └── *_test.cpp              # Component tests
│
├── CMakeLists.txt              # Build config
├── default.conf                # Matching Engine config
└── README.md                   # This file
```

## Building

### Requirements

- C++20 compiler (GCC 11+, Clang 14+)
- CMake 3.23+
- C++ Build System (Ninja was used for this project)

### Build Instructions

```bash
# Clone repository
git clone https://github.com/ArshPanesar/Order-Matching-Engine.git
cd Order-Matching-Engine

# Debug build with tests
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Release build
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Profiling build
cmake -S . -B build/profiling -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/profiling
```

## Benchmarks

### Running Benchmarks

The Release Build contains a `Benchmarks` executable that benchmarks the throughput of the `OrderBook` and the `MatchingEngine`.

```bash
./build/release/benchmarks/Benchmarks
```

There is also a Python script to benchmark the OrderMatchingEngine executable. 

```bash
python benchmarks/pipeline_benchmark.py <tools_executable_path> <engine_executable_path> <config_file_path> 
```

This is to test the performance of the Matching Engine in a realistic setup. This includes:
- **File I/O:** Reading the input events file and Writing an output trades file.
- **Thread Scheduling:** File I/O is executed on separate threads.
- **Matching Engine Processing:** Runs on the Main Thread.
- **SPSC Queue Communication:** Reader Thread pushes OrderEvents, MatchingEngine extracts them, processes the event,  and pushes TradeEvents to another queue where the Writer Thread picks them up.

### Performance Metrics

#### Benchmark Hardware

**CPU:** AMD Ryzen 9 5900HX
**RAM:** 16 GB

#### OrderBook Performance (Storage Only, No Matching Logic)

For 1,000,000 Orders.

**Insertion**

|  Benchmark | Data |
|------------|-----------|
| Time Taken            | 99.242 ms |
| Operations per second | 10,076,333 |

**Removal**

|  Benchmark | Data |
|------------|-----------|
| Time Taken            | 91.516 ms |
| Operations per second | 10,927,085 |

**Search**

|  Benchmark | Data |
|------------|-----------|
| Time Taken                | 62.932 ms |
| Operations per second | 15,890,047 |

#### MatchingEngine Performance (Storage and Matching Logic)

For 1,000,000 Orders Events.

|  Benchmark | Data |
|------------|-----------|
| Time Taken                  | 77.662 ms |
| Orders Processed per second | 12,876,375 |
| Nanoseconds per Order Event | 77.66 ns/op |

#### Pipeline Performance

For 5,000,000 Order Events.

|  Benchmark | Data |
|------------|-----------|
| Total Order Events Processed | 5,000,000 |
| Time Taken  | 0.915635 seconds |
| Order Events processed per second | 5,460,692 |

## Library Architecture

The `LimitOrderBookLib` library implements a high-performance limit order matching engine designed for predictable latency and cache-efficient data access.

The architecture is composed of the following major components.

### MatchingEngine

The `MatchingEngine` is responsible for processing incoming OrderEvents and producing TradeEvents.

It coordinates:
- Extracting events from an input sink
- Executing matching logic
- Updating the `OrderBook`
- Publishing TradeEvents to an output sink

The engine is designed as a single-threaded core for deterministic processing and low latency.

### OrderBook

The `OrderBook` maintains the active orders for a single instrument and enforces price-time priority.

Key responsibilities:

- Insert new orders
- Cancel existing orders
- Amend order quantities
- Match incoming orders against resting liquidity

Orders are organized by `PriceLevel`, where each level maintains FIFO ordering.

### OrderTable

The `OrderTable` maps `OrderID` to `OrderNode*` for O(1) lookup. It uses **Robin Hood hashing** to minimize clustering and long table walks during lookups.

### PriceLevels

Each price level contains a **doubly-linked FIFO list** of orders resting at that price. This preserves strict **time priority** for matching.

A separate **Bitset** is used to quickly locate the best bid and ask prices.

### MemoryAllocator

A custom **free-list memory allocator** is used to allocate a large buffer from the heap. This is used to perform allocations for the `OrderBook`, this includes the `OrderTable`, an `OrderNodePool` to store Order Information, `Bitsets` for Price Levels and flat arrays for `PriceLevels`.

This avoids `new/delete` latency and improves cache locality.

### Event Sinks

The matching engine uses **template-based event sinks** defined via C++20 concepts.

This allows the engine to operate with different I/O strategies without modifying the core matching logic.

Examples include:
- File-based binary event streams
- Mock sinks used for benchmarking
- Threaded pipelines using SPSC queues
