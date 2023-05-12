# Binary Ninja iBoot Loader

A native C++ Binary Ninja view (loader) for SecureROM and iBoot.

## Get Started

Clone the repository, configure the project using CMake, then build:

```sh
cmake -S . -B build -DBN_API_PATH=<...> # -GNinja ...
cmake --build build
```
> Note the `-DBN_API_PATH` argument passed to CMake when configuring the
> project. This should be the path to a copy of the
> [Binary Ninja API](https://github.com/Vector35/binaryninja-api) that you have
> cloned separately.

To install, either copy `libview_iboot.dylib` to your Binary Ninja user
plugins folder, or just run the provided `install` target:

```sh
cmake --build build -t install
```

## Credits

This is a native port of [EliseZeroTwo](https://github.com/EliseZeroTwo) and
[matteyeux](https://github.com/matteyeux)'s
[iBoot loader](https://github.com/EliseZeroTwo/iBoot-Binja-Loader) plugin.
