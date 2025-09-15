# In /home/admin/Documents/VM/arm-toolchain.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0) # Sticking to our true target CPU

set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

# --- THE FIX ---
# Point to the directory containing libuser.a
set(OS_LIB_PATH "/home/admin/Documents/OS/build")

# Force the linker to use libuser.a for EVERY link, including the sanity check.
# This will resolve the "undefined reference to _exit" error for CMake.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${OS_LIB_PATH} -luser" CACHE INTERNAL "")

# --- Compiler Flags ---
# We are compiling for Cortex-M0 as per the original plan.
set(CPU_FLAGS "-mcpu=cortex-m0 -mthumb -g")

# NO -nostdlib. We want the C/C++ libraries. The stubs are in libuser.a.
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -ffreestanding -O2 -Wall")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -ffrestanding -O2 -Wall -fexceptions -frtti")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT}" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT}" CACHE INTERNAL "")

# Boilerplate for cross-compiling
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)