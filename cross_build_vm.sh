#!/bin/bash
set -e

# Go to this script's directory so relative paths work
cd "$(dirname "$0")"

# Project variables
APP_NAME=vm_app
TOOLCHAIN_PREFIX=arm-none-eabi

# Directories
SRC_DIR=src
BUILD_DIR=build
INCLUDE_DIRS=(
    "$SRC_DIR/include"
    "../libuser/include"
)

# Create build directory
mkdir -p "$BUILD_DIR"

# Collect all .c source files (recursively if you like)
C_SOURCES=$(find "$SRC_DIR" -name '*.c')
ASM_SOURCES=$(find "$SRC_DIR" -name '*.s' -o -name '*.S')

# Construct include flags (-I for each include directory)
INCLUDE_FLAGS=""
for inc in "${INCLUDE_DIRS[@]}"; do
    INCLUDE_FLAGS+=" -I$inc"
done

SPEC_FLAGS="-specs=nosys.specs"

# Compiler flags
CFLAGS="-O2 -Wall -Wextra -nostdlib -mcpu=cortex-a9 $SPEC_FLAGS $INCLUDE_FLAGS"

# Assemble and compile
OBJ_FILES=()

echo "Compiling C sources..."
for src in $C_SOURCES; do
    obj="$BUILD_DIR/$(basename "${src%.*}").o"
    ${TOOLCHAIN_PREFIX}-gcc -c $src -o "$obj" $CFLAGS -DVM_DEBUG
    OBJ_FILES+=("$obj")
done

echo "Assembling assembly sources..."
for src in $ASM_SOURCES; do
    obj="$BUILD_DIR/$(basename "${src%.*}").o"
    ${TOOLCHAIN_PREFIX}-as -mcpu=cortex-a9 -o "$obj" "$src"
    OBJ_FILES+=("$obj")
done

FILTERED_OBJS=()
for obj in "${OBJ_FILES[@]}"; do
    base=$(basename "$obj")
    # skip any leftover CMake probe objects or other junk
    if [[ "$base" != CMakeCCompilerId.o && "$base" != CMakeCCompilerId.* ]]; then
        FILTERED_OBJS+=("$obj")
    fi
done
OBJ_FILES=("${FILTERED_OBJS[@]}")

# Link the application
echo "Linking..."
${TOOLCHAIN_PREFIX}-gcc \
    $SPEC_FLAGS \
    -T "$SRC_DIR/$APP_NAME.ld" \
    -o "$BUILD_DIR/$APP_NAME.elf" \
    "${OBJ_FILES[@]}" \
    ../OS/build/libuser.a \
    -nostartfiles -nostdlib -lgcc -Wl,-Map="$BUILD_DIR/output.map"

# Generate binary and .proc
echo "Generating binary and proc files..."
${TOOLCHAIN_PREFIX}-objcopy -O binary "$BUILD_DIR/$APP_NAME.elf" "$BUILD_DIR/$APP_NAME.bin"
../OS/tools/pack_app "$BUILD_DIR/$APP_NAME.bin" "$BUILD_DIR/$APP_NAME.proc"

echo "✅ $APP_NAME built successfully!"
