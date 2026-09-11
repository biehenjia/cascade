cmake_minimum_required(VERSION 3.21)
project(cascade LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT TARGET cr_core::cr_core)
    set(CASCADE_CR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../link" CACHE PATH "CR core source directory")
    if(NOT EXISTS "${CASCADE_CR_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "Set CASCADE_CR_SOURCE_DIR to a CR core checkout, or provide the cr_core::cr_core target.")
    endif()
    add_subdirectory("${CASCADE_CR_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/cr-core")
endif()

find_package(LLVM CONFIG REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Standalone utilities do not propagate CR headers or libraries.
add_library(cascade_llvm INTERFACE)
add_library(cascade::llvm ALIAS cascade_llvm)
target_compile_features(cascade_llvm INTERFACE cxx_std_20)
target_include_directories(cascade_llvm INTERFACE include)
target_include_directories(cascade_llvm SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(cascade_llvm INTERFACE ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(CASCADE_LLVM_UTILITY_LIBS
    core support passes orcjit native nativecodegen)
target_link_libraries(cascade_llvm INTERFACE ${CASCADE_LLVM_UTILITY_LIBS})

add_library(cascade_lower INTERFACE)
add_library(cascade::lower ALIAS cascade_lower)
target_compile_features(cascade_lower INTERFACE cxx_std_20)
target_include_directories(cascade_lower INTERFACE include)
target_include_directories(cascade_lower SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(cascade_lower INTERFACE ${LLVM_DEFINITIONS})
target_link_libraries(cascade_lower INTERFACE cr_core::cr_core)

llvm_map_components_to_libnames(CASCADE_LLVM_LOWER_LIBS core support)
target_link_libraries(cascade_lower INTERFACE ${CASCADE_LLVM_LOWER_LIBS})

# Compile the lowering headers and supply a clangd entry even without tests.
add_library(cascade_tools OBJECT tools/tools.cpp)
target_link_libraries(cascade_tools PRIVATE cascade::lower)

include(CTest)
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    foreach(test IN ITEMS llvm cr symbolic_tape)
        add_executable(cascade_${test}_test tests/${test}.cpp)
        target_link_libraries(cascade_${test}_test PRIVATE cascade::llvm)
        if(NOT test STREQUAL "llvm")
            target_link_libraries(cascade_${test}_test PRIVATE cascade::lower)
        endif()
        add_test(NAME cascade_${test} COMMAND cascade_${test}_test)
    endforeach()
endif()
