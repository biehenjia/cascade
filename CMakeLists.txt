cmake_minimum_required(VERSION 3.20)
project(cascade CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT LLVM_DIR)
    set(LLVM_DIR "/opt/homebrew/opt/llvm/lib/cmake/llvm" CACHE PATH "")
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

llvm_map_components_to_libnames(LLVM_LIBS core support orcjit native passes)

if(NOT TARGET cr_core)
    add_subdirectory(../cr-core ${CMAKE_CURRENT_BINARY_DIR}/cr-core)
endif()

add_library(cascade_codegen INTERFACE)
target_include_directories(cascade_codegen INTERFACE include)
target_link_libraries(cascade_codegen INTERFACE cr_core)
target_include_directories(cascade_codegen INTERFACE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(cascade_codegen INTERFACE ${LLVM_DEFINITIONS})
target_link_libraries(cascade_codegen INTERFACE ${LLVM_LIBS})

enable_testing()
add_subdirectory(tests)
