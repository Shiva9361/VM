cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CROSS_COMPILE "Enable cross-compilation for Cortex M0" OFF)
option(CROSS_COMPILE_A9 "Enable cross-compilation for Cortex A9" OFF)

if(CROSS_COMPILE)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR cortex-m0)

    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
    set(CMAKE_LINKER arm-none-eabi-ld)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m0 -mthumb -specs=nano.specs -Wall -Os -ffunction-sections -fdata-sections ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m0 -mthumb -specs=nano.specs -Wall -Os -ffunction-sections -fdata-sections -fno-rtti")
elseif(CROSS_COMPILE_A9)
    message(STATUS "Configuring for Cortex-A9 (Linux user-space)")

    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)

    set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
    set(CMAKE_ASM_COMPILER arm-linux-gnueabihf-gcc)

    # CPU/ABI flags
    add_compile_options(-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard)
    add_link_options(-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard)

    # ✅ Tell compiler where sysroot is
    set(SYSROOT_PATH "/usr/arm-linux-gnueabihf")

    # Force sysroot flag for both C and C++
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} --sysroot=${SYSROOT_PATH} -Wall -O2 -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${SYSROOT_PATH} -Wall -O2 -ffunction-sections -fdata-sections")

    # Ensure includes are correct
    include_directories(SYSTEM "${SYSROOT_PATH}/include")
endif()




add_executable(vm
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
)

if(CROSS_COMPILE_A9)
    target_sources(vm PRIVATE
        libuser/user_startup_a9.S
        ../OS/apps/libuser/user_syscalls.c
    )
    target_link_options(vm PRIVATE "-T${CMAKE_CURRENT_SOURCE_DIR}/src/user.ld")
    target_include_directories(vm PRIVATE 
        ../OS/apps/libuser/include
    )
endif()


if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(VM_DEBUG)
    add_compile_options(-g)
    message("Debug mode enabled")
endif()

target_include_directories(vm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

