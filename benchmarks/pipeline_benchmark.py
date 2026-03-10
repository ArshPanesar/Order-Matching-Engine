import subprocess
import sys
import time

"""
Required Arguments (In Order): <tools_exec> <engine_exec> <config_file_path>

This script will perform the following steps to benchmark the OrderMatchingEngine executable.
- Read the engine configuration file.
- Use the Tools executable to generate the binary file of OrderEvents.
- Run the OrderMatchingEngine on this binary file.
- Measure the runtime of the OrderMatchingEngine.

This script will measure the total time taken for all operations of OrderMatchingEngine:
- File I/O (Reading the input events file and Writing an output trades file)
- Thread Scheduling (File I/O is executed on separate threads)
- Matching engine processing (Runs on the Main Thread)
- SPSC Queue Communication (Reader Thread pushes OrderEvents, MatchingEngine extracts them, processes the event, 
  and pushes TradeEvents to another queue where the Writer Thread picks them up)
"""


def GenerateEvents(tools_exec, config_file_path, output_file_path):
    print("Generating Order Events...")
    
    subprocess.run(
        [tools_exec, "generate_events", "--config", config_file_path, "--output", output_file_path],
        stdout=subprocess.DEVNULL, # All Prints are discarded for Benchmarking
        stderr=subprocess.DEVNULL,
        check=True
    )


def RunMatchingEngine(engine_exec, input_file_path, output_file_path, config_file_path):
    print("Running Matching Engine...")

    start_time = time.perf_counter()

    subprocess.run(
        [
            engine_exec,
            "--input", input_file_path,
            "--output", output_file_path,
            "--config", config_file_path
        ],
        stdout=subprocess.DEVNULL, # All Prints are discarded for Benchmarking
        stderr=subprocess.DEVNULL,
        check=True
    )

    end_time = time.perf_counter()

    return (end_time - start_time)

def LoadConfig(config_file_path):
    config = {}

    # Parse the Configurations
    with open(config_file_path, "r") as f:
        for line in f:
            line = line.strip()
            key, value = line.split("=")
            config[key.strip()] = int(value.strip())

    return config

def main():
    if len(sys.argv) != 4:
        print("Usage:")
        print("python pipeline_benchmark.py <tools_exe> <engine_exe> <config_file>")
        sys.exit(1)

    # Extract Args
    tools_exec = sys.argv[1]
    engine_exec = sys.argv[2]
    config_file_path = sys.argv[3]

    # Prepare for Benchmarking
    events_file_path = "events.bin"
    trades_file_path = "trades.bin"

    config = LoadConfig(config_file_path)

    num_order_events = config["max_order_events"]

    # Generate events
    GenerateEvents(tools_exec, config_file_path, events_file_path)

    # Run the Matching Engine
    duration_sec = RunMatchingEngine(engine_exec, events_file_path, trades_file_path, config_file_path)

    # Compute Stats
    order_events_per_sec = num_order_events / duration_sec

    print("\nPipeline Benchmark Stats:")
    print(f"Total Order Events Processed: {num_order_events:,}")
    print(f"Duration: {duration_sec:.6f} seconds")
    print(f"OrderEvents/sec: {order_events_per_sec:,.0f}")

if __name__ == "__main__":
    main()