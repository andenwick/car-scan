# CMakeLists.txt — Build configuration for the OBD-II C library
#
# CMake reads this file and generates build instructions for your compiler.
# Think of it as a recipe: "compile these .c files into a library, with
# these settings, and also build the tests."

cmake_minimum_required(VERSION 3.15)
project(obd C)  # Project name is "obd", language is C

# Use C11 standard (modern C with good features like designated initializers)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── The static library ──────────────────────────────────────────────────
# A "static library" (.lib on Windows, .a on Linux) is a bundle of compiled
# code that gets linked directly into your executable. No DLL needed.
#
# We list every .c source file that makes up the library.
add_library(obd STATIC
    src/hex_utils.c
    src/elm327.c
    src/pid.c
    src/sensor.c
    src/dtc.c
    src/vin.c
)

# Tell the compiler where to find our header files.
# PUBLIC means anything that links against this library also gets these paths.
# "include" directory contains our public headers (obd/obd.h, obd/obd_types.h)
# "src" directory contains private headers (elm327.h, pid.h, etc.)
target_include_directories(obd
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── Compiler warnings ──────────────────────────────────────────────────
# Strict warnings catch bugs at compile time instead of runtime.
# MSVC uses /W4 (warning level 4) and /WX (treat warnings as errors).
# GCC/Clang use -Wall -Wextra -Werror (same idea, different syntax).
# On MSVC, suppress warnings about standard C functions like strncpy/snprintf.
# Microsoft pushes their _s variants, but we use standard C for portability.
if(MSVC)
    target_compile_definitions(obd PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(obd PRIVATE /W4 /WX)
else()
    target_compile_options(obd PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

# ── Tests ───────────────────────────────────────────────────────────────
# enable_testing() turns on CTest support so we can run tests with "ctest"
# Skip tests when building for Android (NDK defines ANDROID automatically)
if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
