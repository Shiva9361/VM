cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vm
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
    libuser/user_startup_a9.S
    ../OS/apps/libuser/user_syscalls.c
)

target_include_directories(vm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ../OS/apps/libuser/include
)

# Linker script and link options
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/user.ld)
target_link_options(vm PRIVATE
    -T ${LINKER_SCRIPT}
    --specs=nano.specs
    -Wl,--gc-sections
    -nostartfiles
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(VM_DEBUG)
    add_compile_options(-g)
    message("Debug mode enabled")
endif()
