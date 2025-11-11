#!/bin/bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build
make
cd ..
cp build/vm_executable vm_c
chmod +x vm
echo "VM built successfully."