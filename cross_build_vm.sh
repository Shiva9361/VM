#!/bin/bash
rm -rf build_m0
mkdir build_m0
cmake -B build_m0 -DCMAKE_BUILD_TYPE=Debug -DCROSS_COMPILE=ON
cd build_m0
make
cd ..
cp build_m0/vm vm_m0
chmod +x vm_m0
echo "VM for Cortex-M0 built successfully."
