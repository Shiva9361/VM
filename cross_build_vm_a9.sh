#!/bin/bash
set -e
rm -rf build_a9
cmake -B build_a9 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake
cmake --build build_a9 -j$(nproc)
cp build_a9/vm vm_a9
chmod +x vm_a9
echo " VM for Cortex-A9 built successfully."
