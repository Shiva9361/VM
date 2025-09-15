# build_arm.sh (in your friend's VM project folder)
#!/bin/bash
rm -rf build_arm
mkdir build_arm
# Tell CMake to use our new toolchain file
cmake -B build_arm -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake

cd build_arm
make

# Convert the final ELF into a raw binary for our OS to load
arm-none-eabi-objcopy -O binary vm.elf vm.bin
cd ..
echo "ARM VM binary created at build_arm/vm.bin"