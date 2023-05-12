# Binary Ninja iBoot Loader

A native C++ Binary Ninja view (loader) for SecureROM and iBoot.

## Get Started

First, clone the repository and the API submodule:

```sh
git clone --recursive git@github.com:jonpalmisc/bn_cpp_template.git
```

Next, configure the project using CMake, then build:

```sh
cmake -S . -B build # -GNinja ...
cmake --build build
```

To install, either copy `libview_iboot.dylib` to your Binary Ninja user
plugins folder, or just run the provided `install` target:

```sh
cmake --build build -t install
```

## Credits

This is a native subset of [EliseZeroTwo](https://github.com/EliseZeroTwo) and
[matteyeux](https://github.com/matteyeux)'s
[iBoot loader](https://github.com/EliseZeroTwo/iBoot-Binja-Loader) plugin.
