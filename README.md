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

## Building and Running PPFS with FUSE

After building the project, you’ll find two binaries:

1. **`mkfs_ppfs`** – used for creating and formatting a filesystem image.  
2. **`mount_ppfs`** – used for mounting an existing filesystem image via FUSE.

Both binaries are located in:

```
./build/debug/fuse_exec/
```

### Creating a filesystem image

---

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

### Configuration file format

---

Filesystem configuration is provided using a plain text file with `key = value` pairs.
Each configuration option must be specified on a separate line.

#### General rules

- Lines starting with `#` or `//` are treated as comments and ignored
- Inline comments are allowed (everything after `#` or `//` is ignored)
- Whitespace around keys and values is ignored
- Each line must contain exactly one `=` character
- Unknown keys cause a configuration error
- Missing required fields cause a configuration error


#### Required fields

The following fields are always required:

#### `total_size`
- **Type:** `uint64_t`
- **Description:** Total size of the filesystem image in bytes, must be a multiple of block size
- **Example:**
```

total_size = 1048576

```

#### `average_file_size`
- **Type:** `uint64_t`
- **Description:** Expected average file size, used to tune internal filesystem structures
- **Example:**
```

average_file_size = 4096

```

#### `block_size`
- **Type:** `uint32_t`
- **Description:** Block size in bytes (must be a power of two)
- **Example:**
```

block_size = 512

```

#### `ecc_type`
- **Type:** enum
- **Allowed values:** `none`, `crc`, `reed_solomon`, `parity`, `hamming`
- **Description:** Error correction mechanism used by the filesystem
- **Example:**
```

ecc_type = crc

```

---

### Conditionally required fields

Some fields are required only for specific `ecc_type` values.

#### `crc_polynomial`
- **Type:** `unsigned long int`
- **Required when:** `ecc_type = crc`
- **Description:** Polynomial used for CRC error detection. Each bit represents a coefficient in CRC polynomial, with the most significant bit corresponding to the highest degree. The constant term (`+1`) is implicit and is not encoded in this value.
- **Format:** Decimal or hexadecimal (`0x` prefix supported)
- **Examples:**
```

crc_polynomial = 0x9960034c
crc_polynomial = 257

```

#### `rs_correctable_bytes`
- **Type:** `uint32_t`
- **Required when:** `ecc_type = reed_solomon`
- **Description:** Number of bytes that can be corrected per block
- **Example:**
```

rs_correctable_bytes = 3

```

---

### Optional fields

#### `use_journal`
- **Type:** `bool`
- **Allowed values:** `true`, `false`, `1`, `0`
- **Default:** `false`
- **Description:** Enables or disables journaling support
- **Example:**
```

use_journal = false

````

---

### Example configuration file

```ini
# Example filesystem configuration

total_size = 1048576
average_file_size = 4096
block_size = 512

ecc_type = crc
crc_polynomial = 0x9960034c

use_journal = false
````

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
./build/release/usage_simulator/usage_simulator <path_to_config_file> <logs_directory>
```

There is also python simulation runner. Simulation runner creates simulation scenarios defined in the script
and runs them in parallel. Then it saves useful plots to `simulator_plots/` directory
To run it:

1. build program
2. ```bash
   ./.venv/bin/python3 ./simulation_runner/runner.py

```

## Run fuse benchmark

In order to measure performance of FUSE I/O operations for different ECC, you can use a script that automates fio runs 
and saves results to `fio_plots/` directory
To run it:
1. build program with `release` preset
2. 2. ```bash
   ./.venv/bin/python3 ./fuse_benchmark/runner.py

```
