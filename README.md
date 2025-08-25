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

Now you should see files inside `/tmp/ppfs`, e.g.:

```bash
ls mnt
# hello.txt
cat mnt/hello.txt
# Hello, world!
```

To unmount:

```bash
fusermount3 -u /tmp/ppfs
```

## 🧪 Run Tests

```bash
ctest --preset debug
```
