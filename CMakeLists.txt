cmake_minimum_required(VERSION 3.15)
project(CalcOS
    VERSION 0.1.0
    LANGUAGES C CXX
)

# C11 for _Static_assert and _Thread_local. C++17 for the plugin loader.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# keep binaries and libs separate from source tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SIMD options. AVX2+SSE2 on by default. NEON off by default (ARM cross-compile).
# CALC_TINY_MEM: size-optimize for <64KB targets (Cortex-M, DOS, retro hardware).
option(CALC_ENABLE_AVX2 "Compile with AVX2 instruction set support" ON)
option(CALC_ENABLE_SSE2 "Compile with SSE2 instruction set support" ON)
option(CALC_ENABLE_NEON "Compile with ARM NEON instruction set support" OFF)
option(CALC_TINY_MEM "Compile for ultra-low-memory environments (16KB/24KB/32KB)" OFF)

# glob source and headers for the calc library. explicit lists would be better but
# this codebase has too many files to enumerate. at least it's consistent.
file(GLOB_RECURSE KERNEL_SOURCES
    "Kernel/Core/*.c"
    "Kernel/Operations/*.c"
    "Kernel/State/*.c"
)
file(GLOB_RECURSE INFRASTRUCTURE_SOURCES
    "Infrastructure/Utils/*.c"
    "Infrastructure/Config/*.c"
)
file(GLOB_RECURSE APPLICATION_SOURCES
    "Application/Input/*.c"
    "Application/Output/*.c"
    "Application/Interface/*.c"
    "Application/UI/*.c"
)
file(GLOB SRC_SOURCES
    "src/*.c"
)

set(CALC_SOURCES
    ${KERNEL_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${APPLICATION_SOURCES}
    ${SRC_SOURCES}
)

file(GLOB_RECURSE KERNEL_HEADERS
    "Kernel/Core/*.h"
    "Kernel/Operations/*.h"
    "Kernel/State/*.h"
)
file(GLOB_RECURSE INFRASTRUCTURE_HEADERS
    "Infrastructure/Utils/*.h"
    "Infrastructure/Config/*.h"
)
file(GLOB_RECURSE APPLICATION_HEADERS
    "Application/Input/*.h"
    "Application/Output/*.h"
    "Application/Interface/*.h"
    "Application/UI/*.h"  # was *.c here -- that's a bug, headers only
)
file(GLOB SRC_HEADERS
    "src/*.h"
    "include/calc/*.h"
    "include/calc/*.hpp"
)

set(CALC_HEADERS
    ${KERNEL_HEADERS}
    ${INFRASTRUCTURE_HEADERS}
    ${APPLICATION_HEADERS}
    ${SRC_HEADERS}
)

# ---------------------------------------------------------
# Static Library
# ---------------------------------------------------------
add_library(calc_static STATIC ${CALC_SOURCES} ${CALC_HEADERS})
set_target_properties(calc_static PROPERTIES
    OUTPUT_NAME "calc"
    CLEAN_DIRECT_OUTPUT 1
)

# ---------------------------------------------------------
# Shared Library
# ---------------------------------------------------------
add_library(calc_shared SHARED ${CALC_SOURCES} ${CALC_HEADERS})
set_target_properties(calc_shared PROPERTIES
    OUTPUT_NAME "calc"
    CLEAN_DIRECT_OUTPUT 1
)

# Include directories configuration
foreach(target calc_static calc_shared)
    # include root so #include paths resolve from project root
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    # warnings: W3 on MSVC (W4 breaks third-party headers). Wall+Wextra+Werror on GCC/Clang.
    # -O3: let the compiler do its job. the SIMD dispatch is the bottleneck anyway.
        target_compile_options(${target} PRIVATE /W3 /wd4100 /wd4210)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -O3)
    endif()

    # Apply AVX2 configurations
    if(CALC_ENABLE_AVX2)
        target_compile_definitions(${target} PRIVATE USE_AVX2=1)
        if(MSVC)
            target_compile_options(${target} PRIVATE /arch:AVX2)
        else()
            target_compile_options(${target} PRIVATE -mavx2)
        endif()
    endif()

    # Apply SSE2 configurations
    if(CALC_ENABLE_SSE2)
        target_compile_definitions(${target} PRIVATE USE_SSE2=1)
        if(MSVC)
            if(CMAKE_SIZEOF_VOID_P EQUAL 4)
                target_compile_options(${target} PRIVATE /arch:SSE2)
            endif()
        else()
            target_compile_options(${target} PRIVATE -msse2)
        endif()
    endif()

    # Apply NEON configurations
    if(CALC_ENABLE_NEON)
        target_compile_definitions(${target} PRIVATE USE_NEON=1)
        if(NOT MSVC)
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM")
                target_compile_options(${target} PRIVATE -mfpu=neon)
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AARCH64")
                target_compile_options(${target} PRIVATE -march=armv8-a+simd)
            endif()
        endif()
    endif()
endforeach()


# ---------------------------------------------------------
# Test Executable
# ---------------------------------------------------------
add_executable(test_stats_rng Tests/Unit/TestStatsRNG.c)
target_link_libraries(test_stats_rng PRIVATE calc_static)
target_include_directories(test_stats_rng PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_executable(test_parse Tests/Unit/temp_test_parse.c)
target_link_libraries(test_parse PRIVATE calc_static)
target_include_directories(test_parse PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_executable(calc_terminal
    Platform/Host/main_host.c
    Platform/Host/host_term.c
    Platform/Host/host_sdl.c
    Platform/Host/host_input.c
)
target_link_libraries(calc_terminal PRIVATE calc_static)
target_include_directories(calc_terminal PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


add_executable(test_complex_calc Tests/Unit/test_complex_calc.c)
target_link_libraries(test_complex_calc PRIVATE calc_static)
target_include_directories(test_complex_calc PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(test_complex_calc PRIVATE HOST_APP=1)

add_executable(test_cpp_wrapper Tests/Unit/TestCppWrapper.cpp)
target_link_libraries(test_cpp_wrapper PRIVATE calc_static)
target_include_directories(test_cpp_wrapper PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(test_cpp_wrapper PRIVATE HOST_APP=1)

add_executable(test_plugin_system Tests/Unit/TestPluginSystem.cpp)
target_link_libraries(test_plugin_system PRIVATE calc_static)
target_include_directories(test_plugin_system PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(test_plugin_system PRIVATE HOST_APP=1)

# Dynamic Plugin Compilation & Staging Target
add_library(sample_plugin SHARED Tests/Unit/SamplePlugin.cpp)
target_link_libraries(sample_plugin PRIVATE calc_static)
target_include_directories(sample_plugin PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
add_custom_command(TARGET sample_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:sample_plugin>/plugins
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sample_plugin> $<TARGET_FILE_DIR:sample_plugin>/plugins/
)
add_dependencies(test_plugin_system sample_plugin)

add_executable(test_benchmark Tests/Performance/test_benchmark.c)
target_link_libraries(test_benchmark PRIVATE calc_static)
target_include_directories(test_benchmark PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(test_benchmark PRIVATE HOST_APP=1)
if(MSVC)
    target_compile_options(test_benchmark PRIVATE /W3 /wd4100 /wd4210 /O2)
endif()

# Apply CALC_TINY_MEM size optimizations globally to all targets
if(CALC_TINY_MEM)
    foreach(tgt calc_static calc_shared calc_terminal test_stats_rng test_parse test_complex_calc test_benchmark test_cpp_wrapper)
        if(TARGET ${tgt})
            target_compile_definitions(${tgt} PRIVATE CALC_TINY_MEM=1)
            if(MSVC)
                target_compile_options(${tgt} PRIVATE /O1 /Os /Gy /Gw)
                target_link_options(${tgt} PRIVATE /OPT:REF)
            else()
                target_compile_options(${tgt} PRIVATE -Os -ffunction-sections -fdata-sections)
                target_link_options(${tgt} PRIVATE -Wl,--gc-sections)
                set_target_properties(${tgt} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
            endif()
        endif()
    endforeach()
endif()

