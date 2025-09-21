cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX C)

add_executable(vm.elf
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
)

target_include_directories(vm.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/user.ld)
target_link_options(vm.elf PRIVATE
    -T ${LINKER_SCRIPT}
    --specs=nano.specs
)

# Link using the absolute path to the library. This is the most reliable method.
target_link_libraries(vm.elf PRIVATE "/home/mokshith/Desktop/OS/build/libuser.a")

add_custom_command(TARGET vm.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:vm.elf>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:vm.elf> ../vm.bin
    COMMENT "--- Built VM for Cortex-M0: ../vm.bin ---"
)