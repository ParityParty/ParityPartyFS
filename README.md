# ParityPartyFS (ppfs)

A toy filesystem implemented with **FUSE** and **C++23**, built using [Fusepp](https://github.com/jachappell/Fusepp).

## 📦 Dependencies

On Ubuntu/Debian, install required packages:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libfuse3-dev fuse3     
```

## ⚡️ Build

We use **CMake presets** with **Ninja**:

```bash
cmake --preset debug      # configure (Debug build)
cmake --build --preset debug   # build everything
```

Other presets you can use:

- `debug` → Debug mode (with tests, symbols).
- `release` → Release mode (optimized).
- `freertos_debug`
- `freertos_release`

## ▶️ Run the filesystem

After building, you’ll find the binary at:

```
build/debug/exec/ppfs_bin
```

To mount it:

```bash
mkdir mnt
./build/debug/exec/ppfs_bin mnt
```

Now you can add files to mnt

```bash
echo "abc" > mnt/file
cat mnt/file
# abc
```

To unmount:

```bash
umount mnt
```

## 🧪 Run Tests

```bash
ctest --preset debug
```

## Run benchmark

```bash
./build/release/performance_tests/performance_tests --benchmark_counters_tabular=true
```

## Run Usage Simulator

Usage simulator creates filesystem instance and runs threads doing operations on it. There are random bit flips
performed during the simulation.
There is a configuration file allowing to modify simulation parameters. Example configuration can be found in
`usage_simulator/simulation_config.txt`

```bash
./build/release/usage_simulator/usage_simulator <path_to_config_file> <logs_directory>
```

There is also python simulation runner. Simulation runner creates simulation scenarios defined in the script
and runs them in parallel. Then it saves useful plots to `plots/` directory
To run it:

1. build program
2. ```bash
   ./.venv/bin/python3 ./simulation_runner/runner.py

```
