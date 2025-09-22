#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

mkdir -p build
cd build

# Configure using the toolchain file
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug

# Build the project
make VERBOSE=1

cp vm.elf ../vm.elf

cd ..

arm-none-eabi-objcopy -O binary "vm.elf" "vm.bin"

TOOLS_DIR="../OS/tools"

cc -O2 -o "$TOOLS_DIR/pack_app" "$TOOLS_DIR/pack_app.c"

# wrap as .proc (header + body)
"$TOOLS_DIR/pack_app" "vm.bin" "vm.proc"


# The copy command is now redundant since the post-build step in CMake
# already places vm.bin in the parent directory.
# cp vm.bin ../

echo "VM built successfully."