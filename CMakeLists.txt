cmake_minimum_required(VERSION 3.10)
project(SecureInput C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

# Options
option(DEBUG "Enable debug mode" OFF)
option(SANITIZE "Enable AddressSanitizer" OFF)
option(OPTIMIZE "Enable optimizations" OFF)

# Include directories
include_directories(include tests)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fPIC")
set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64 -DPIC")

if(DEBUG)
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -g")
endif()

if(SANITIZE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(OPTIMIZE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
endif()

# Library
add_library(secureinput SHARED
    lib/secureinput.c
    lib/secureinput.asm)

add_library(secureinput_static STATIC
    lib/secureinput.c
    lib/secureinput.asm)

# Main executable
add_executable(secureinput_main examples/main.c)
target_link_libraries(secureinput_main secureinput m)

# Benchmark executable
add_executable(bench_secureinput tests/bench_secureinput.c)
target_link_libraries(bench_secureinput secureinput m)

# Constant-time test executable
add_executable(test_const_time tests/test_const_time.c)
target_link_libraries(test_const_time secureinput m)

# Install
install(TARGETS secureinput secureinput_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(FILES include/secureinput.h DESTINATION include)

# Custom targets
add_custom_target(run_test_const_time
    COMMAND test_const_time
    DEPENDS test_const_time)