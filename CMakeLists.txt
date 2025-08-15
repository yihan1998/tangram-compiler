cmake_minimum_required(VERSION 3.20.0)
project(edamlir LANGUAGES C CXX)

# Enable C++17 standard
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Enable export of compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define build and binary directories
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR})
set(LLVM_RUNTIME_OUTPUT_INTDIR ${PROJECT_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${PROJECT_BINARY_DIR}/lib)

set(LLVM_DIR "${CMAKE_SOURCE_DIR}/install/lib/cmake/llvm")
set(MLIR_DIR "${CMAKE_SOURCE_DIR}/install/lib/cmake/mlir")

# Find MLIR and LLVM
find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set module paths for MLIR and LLVM
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

# Include LLVM and MLIR utilities
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

# Include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)

# Link directories
link_directories(${LLVM_BUILD_LIBRARY_DIR})

# Add LLVM definitions
add_definitions(${LLVM_DEFINITIONS})

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Subdirectories for your project components
add_subdirectory(include)
add_subdirectory(lib)
# add_subdirectory(tests)
add_subdirectory(tools)

# Optional: If third-party dependencies are needed
if(EXISTS "${CMAKE_SOURCE_DIR}/third-party")
    add_subdirectory(third-party)
endif()