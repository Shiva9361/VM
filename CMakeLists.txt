cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX C)

# --- Define VM Source Files ---
add_executable(vm.elf
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
)

target_include_directories(vm.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# --- Link the VM ---
# The user program needs its own simple linker script.
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/user.ld)

# The toolchain file already handles linking libuser.a.
# We just add the linker script and standard library specs.
target_link_options(vm.elf PRIVATE
    -T ${LINKER_SCRIPT}
    --specs=nano.specs
)

# --- Post-build Commands ---
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)
add_custom_command(TARGET vm.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:vm.elf>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:vm.elf> ../vm.bin # Put bin file in root
    COMMENT "--- Built VM for Cortex-M0: ../vm.bin ---"
)