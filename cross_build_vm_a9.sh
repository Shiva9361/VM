#!/bin/bash
cd "$(dirname "$0")"
rm -rf build_a9
mkdir build_a9
cmake -B build_a9 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=arm-cortex-a9-toolchain.cmake
cd build_a9
make
cd ..
cp build_a9/vm vm_a9
chmod +x vm_a9
echo "VM for Cortex-A9 built successfully."
