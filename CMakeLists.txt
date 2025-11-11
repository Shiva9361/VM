cmake_minimum_required(VERSION 3.10)
project(VM_Project C ASM)

# --- Toolchain / paths (adjust if needed) ---
set(TOOLCHAIN_PATH "/media/shiva/1DC8ABD92E3661741/CSD/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux/gcc-arm-none-eabi-10.3-2021.10/bin")
set(TOOLCHAIN_PREFIX "arm-none-eabi" CACHE STRING "Cross toolchain prefix")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc")
set(OBJCOPY "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy")

# Path to prebuilt libuser.a and pack_app (relative to this CMakeLists.txt)
set(LIBUSER_PATH "${CMAKE_SOURCE_DIR}/../OS/build/libuser.a" CACHE FILEPATH "Path to libuser.a")
set(PACK_APP "${CMAKE_SOURCE_DIR}/../OS/tools/pack_app" CACHE PATH "Path to pack_app tool")

# Linker script (adjust location or override via -DLINKER_SCRIPT=...)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/user.ld" CACHE FILEPATH "Linker script for final link")
if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}\nPlease set -DLINKER_SCRIPT=/path/to/user.ld")
else()
    message(STATUS "Using linker script: ${LINKER_SCRIPT}")
endif()
# --- language / standards ---
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
enable_language(ASM)

# --- include directories ---
include_directories("${CMAKE_SOURCE_DIR}/src/include")

# --- sources (update if you add/remove files) ---
set(SOURCES
    src/common.c
    src/hash_map.c
    src/main.c
    src/object_factory.c
    src/string.c
    src/vector.c
    src/VM.c
    libuser/user_startup_a9.S
    ${CMAKE_SOURCE_DIR}/../OS/apps/libuser/user_syscalls.c
)

# --- target ---
add_executable(vm_executable ${SOURCES})

# --- compile flags (mimic your script flags) ---
target_compile_options(vm_executable PRIVATE
    -O2
    -Wall
    -Wextra
    -nostdlib
    -mcpu=cortex-a9
    -ffreestanding
)

# Debug mode behavior
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(vm_executable PRIVATE VM_DEBUG)
    target_compile_options(vm_executable PRIVATE -g)
    message(STATUS "Debug mode enabled")
endif()

# --- import the prebuilt libuser.a and link it ---
add_library(libuser STATIC IMPORTED)
set_target_properties(libuser PROPERTIES IMPORTED_LOCATION "${LIBUSER_PATH}")
target_link_libraries(vm_executable PRIVATE libuser)

# --- linker options (map file, no stdstart, use provided linker script) ---
# Map file will be placed in the build directory like your script did.
set(MAP_FILE "${CMAKE_BINARY_DIR}/build_output.map")
target_link_options(vm_executable PRIVATE
    -T"${LINKER_SCRIPT}"
    -nostartfiles
    -nostdlib
    -lgcc
    -Wl,-Map=${MAP_FILE}
)

# --- post-build: objcopy -> .bin and pack_app -> .proc (mimic script) ---
set(BIN_OUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")
set(PROC_OUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.proc")

add_custom_command(TARGET vm_executable POST_BUILD
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:vm_executable> ${BIN_OUT}
    COMMAND ${PACK_APP} ${BIN_OUT} ${PROC_OUT}
    COMMENT "Generating raw binary and .proc (objcopy -> pack_app)"
    VERBATIM
)

# --- helpful summary messages ---
message(STATUS "Cross compiler prefix: ${TOOLCHAIN_PREFIX}")
message(STATUS "Using libuser at: ${LIBUSER_PATH}")
message(STATUS "pack_app tool: ${PACK_APP}")
message(STATUS "Linker script: ${LINKER_SCRIPT}")
message(STATUS "Binary output: ${BIN_OUT}")
message(STATUS "Proc output: ${PROC_OUT}")
