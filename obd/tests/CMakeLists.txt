# tests/CMakeLists.txt — Build configuration for test executables
#
# Each test is a standalone executable with its own main().
# We loop over all test modules, create an exe for each, link it
# against our obd library, and register it with ctest.

# List of test module names (matches test_*.c filenames)
set(TEST_MODULES
    hex_utils
    elm327
    pid
    sensor
    dtc
    vin
)

# For each module, create a test executable and register it with ctest.
# Example: "hex_utils" becomes executable "test_hex_utils" built from "test_hex_utils.c"
foreach(module ${TEST_MODULES})
    set(test_name "test_${module}")

    # Create an executable from the test source file
    add_executable(${test_name} ${test_name}.c)

    # Link against our obd library (gives access to all obd functions + headers)
    target_link_libraries(${test_name} PRIVATE obd)

    # Also include the tests directory itself (for test_data.h)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Strict warnings for tests too
    if(MSVC)
        target_compile_definitions(${test_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${test_name} PRIVATE /W4 /WX)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()

    # Register with ctest — "ctest --output-on-failure" will run all of these
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
