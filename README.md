# Contractor

## Building

### Prerequisites

- A cpp17 conform compiler
- cmake v3.15 or newer
- Boost (headers + program_options) - at least v1.67 (though a workaround for earlier versions is integrated in the build system), better v1.70 or newer
- An active internet connection (only needed for first build)

If you don't have Boost installed on your system or the version installed is not recent enough, you can use the `use-bundled-boost` option when invoking cmake.
This will download and build a recent Boost automatically when running cmake. In order to make use of this option, execute `git submodule update --init` first
and then invoke cmake as `cmake -Duse-bundled-boost=ON ..`.

### Instructions

Start out at the repository's root directory.

1. Create a build directory: `mkdir build`
2. Switch into that directory: `cd build`
3. Invoke cmake: `cmake ..`
4. Build from the generated build files: `cmake --build .` (or use native build system directly, e.g. on Unix `make`)


## Testing

Build the application with tests enabled:
```bash
cmake -Dtests=ON ..
cmake --build .
```

Then run the test cases:
```bash
ctest --output-on-failure
```

