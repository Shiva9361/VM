# In /home/mokshith/Desktop/VM/arm-toolchain.cmake

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# Define identical CPU flags as the OS project
set(CPU_FLAGS "-mcpu=cortex-m0 -mthumb")

# Set the compiler flags for this project
set(CMAKE_C_FLAGS "${CPU_FLAGS} -g -Wall -Os -ffunction-sections -fdata-sections" CACHE INTERNAL "")

# --- THIS IS THE LINE TO FIX ---
# Enable C++ exceptions and RTTI for the VM project's code
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -g -Wall -Os -ffunction-sections -fdata-sections -frtti -fexceptions" CACHE INTERNAL "")

set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -g" CACHE INTERNAL "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)