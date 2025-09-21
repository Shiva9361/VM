#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

mkdir -p build
cd build

# Configure using the toolchain file
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake

# Build the project
make

# The copy command is now redundant since the post-build step in CMake
# already places vm.bin in the parent directory.
# cp vm.bin ../

echo "VM built successfully."