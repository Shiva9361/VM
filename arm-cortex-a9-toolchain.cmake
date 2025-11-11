# arm-cortex-a9-toolchain.cmake

# Set the system and processor
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-a9)

# Set the toolchain compilers
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# Set the compiler flags
set(CPU_FLAGS "-mcpu=cortex-a9 -marm")
set(COMMON_FLAGS "-Wall -Os -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS "${CPU_FLAGS} ${COMMON_FLAGS}" CACHE STRING "C compiler flags")
# C++ flags are removed as the project is now C-only
# set(CMAKE_CXX_FLAGS "${CPU_FLAGS} ${COMMON_FLAGS} -fno-rtti -fno-exceptions" CACHE STRING "C++ compiler flags")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}" CACHE STRING "Assembler flags")

# Set the linker flags
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections" CACHE STRING "Linker flags")

# Avoid try-compile running on host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)