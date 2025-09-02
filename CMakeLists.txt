cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CROSS_COMPILE "Enable cross-compilation for Cortex M0" OFF)

if(CROSS_COMPILE)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR cortex-m0)

    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
    set(CMAKE_LINKER arm-none-eabi-ld)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m0 -mthumb -specs=nano.specs -Wall -Os -ffunction-sections -fdata-sections ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m0 -mthumb -specs=nano.specs -Wall -Os -ffunction-sections -fdata-sections -fno-rtti")
endif()

add_executable(vm
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(VM_DEBUG)
    add_compile_options(-g)
    message("Debug mode enabled")
endif()

target_include_directories(vm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
