cmake_minimum_required(VERSION 3.12)
project(VM LANGUAGES CXX C ASM)

# Define user library sources
# set(LIBUSER_SOURCE_FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/libuser/user_syscalls.c
#     ${CMAKE_CURRENT_SOURCE_DIR}/libuser/user_startup.S
# )

# Create static library target "user"
# add_library(user STATIC ${LIBUSER_SOURCE_FILES})

# Apply Cortex-M0 flags and specs for the user library
# target_compile_options(user PRIVATE
#     -mcpu=cortex-m0
#     -mthumb
#     -ffunction-sections
#     -fdata-sections
#     --specs=nosys.specs
#     --specs=nano.specs
# )

# # # If needed, apply ASM flags explicitly (optional)
# target_compile_options(user PRIVATE
#     $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m0 -mthumb -g>
# )

# target_include_directories(user PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/libuser
# )

# Now define the executable
add_executable(vm.elf
    src/main.cpp
    src/VM.cpp
    src/object_factory.cpp
    libuser/user_startup.S
    libuser/user_syscalls.c
)

target_include_directories(vm.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# Linker script and link options
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/user.ld)
target_link_options(vm.elf PRIVATE
    -T ${LINKER_SCRIPT}
    --specs=nano.specs
    -Wl,--gc-sections
    -nostartfiles
)

# target_link_options(vm.elf PRIVATE -Wl,--whole-archive)
# target_link_libraries(vm.elf PRIVATE user)
# target_link_options(vm.elf PRIVATE -Wl,--no-whole-archive)


# Post build commands
add_custom_command(TARGET vm.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:vm.elf>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:vm.elf> ${CMAKE_CURRENT_SOURCE_DIR}/vm.bin
    COMMENT "--- Built VM for Cortex-M0: vm.bin ---"
)
