#!/bin/bash
rm -r build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build
make
cd ..
cp build/vm vm
chmod +x vm
echo "VM built successfully."