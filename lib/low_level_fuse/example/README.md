To build:

```sh
cd lib/low_level_fuse/example/
mkdir build
cd build
cmake ..
cmake --build .
```
To run:

```sh
mkdir mnt
./examplefs mnt
```

To unmount:

```sh
fusermount -u mnt
```