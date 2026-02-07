# CMakeLists.txt — NDK build for the CarScan JNI bridge.
#
# Pulls in the existing OBD-II C library via add_subdirectory and
# builds a shared library (carscan-native.so) containing the JNI bridge.

cmake_minimum_required(VERSION 3.22.1)
project(carscan-jni C)

# Path to the OBD-II C library (5 levels up from cpp/ to CarScan root)
set(OBD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../obd)

# Pull in the existing OBD library (compiles all 6 .c files as a static lib)
add_subdirectory(${OBD_DIR} ${CMAKE_CURRENT_BINARY_DIR}/obd)

# Build the JNI bridge as a shared library (.so)
add_library(carscan-native SHARED jni_bridge.c)

# Link against our OBD static lib + Android log library
target_link_libraries(carscan-native PRIVATE obd log)
