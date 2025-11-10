cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES C ASM)

add_executable(vm
    src/main.c
    src/VM.c
    src/object_factory.c
    libuser/user_startup_a9.S
    ../OS/apps/libuser/user_syscalls.c
)

target_include_directories(vm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/../OS/apps/libuser/include
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
    add_compile_options(-g -std=c99) # Use C99 standard
    message("Debug mode enabled")
endif()

