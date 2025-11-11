#!/bin/bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build
make
cd ..
cp build/vm_executable vm
chmod +x vm
echo "VM built successfully."