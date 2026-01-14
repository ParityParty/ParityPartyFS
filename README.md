# ParityPartyFS (ppfs)

A filesystem implemented with **FUSE** and **C++23**, built using [Fusepp](https://github.com/jachappell/Fusepp).

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

## Building and Running PPFS

After building the project, you’ll find **two binaries**:

1. **`mkfs_ppfs`** – used for creating and formatting a filesystem image.  
2. **`mount_ppfs`** – used for mounting an existing filesystem image via FUSE.

Both binaries are located in:

```
./build/debug/fuse_exec/
```

---

### Creating a filesystem image

To create a new filesystem image, run:

```bash
./build/debug/fuse_exec/mkfs_ppfs <config_file> <disk_file>
````

* `<disk_file>` – path to the image file that will store the filesystem (created if it doesn’t exist).
* `<config_file>` – path to a configuration file specifying filesystem parameters.

The example configuration file is available at:

```
fuse_exec/example_config.txt
```

Example:

```bash
./build/debug/fuse_exec/mkfs_ppfs ppfs.img fuse_exec/example_config.txt
```

---


### Mounting a filesystem

To mount an existing filesystem image, run:

```bash
./build/debug/fuse_exec/mount_ppfs <disk_file> <mount_point> [-- fuse_options]
```

* `<disk_file>` – path to the filesystem image to mount.
* `<mount_point>` – path to an **existing, empty directory** that will serve as the mount point.
* `fuse_options` – optional FUSE parameters (for example `-f` for foreground or `-d` for debug logs). Must be passed **after `--`**.

Example:

```bash
mkdir -p mnt
./build/debug/fuse_exec/mount_ppfs ppfs.img mnt -- -f -d
```
---

### Using the filesystem

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
./build/release/usage_simulator/usage_simulator <path_to_config_file>
```