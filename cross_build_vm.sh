#!/bin/bash
rm -rf build
mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCROSS_COMPILE=ON
cd build
make
cd ..
cp build/vm vm
chmod +x vm
echo "VM built successfully."
