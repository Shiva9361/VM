cmake_minimum_required(VERSION 3.10)

project(VM_Project C)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories for headers
include_directories(src/include)

# Source files
set(SOURCES
    src/common.c
    src/hash_map.c
    src/main.c
    src/object_factory.c
    src/string.c
    src/vector.c
    src/VM.c
)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(VM_DEBUG)
    add_compile_options(-g)
    message("Debug mode enabled")
endif()
# Add the executable
add_executable(vm_executable ${SOURCES})
