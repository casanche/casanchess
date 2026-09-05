cmake_minimum_required(VERSION 3.20)
project(Casanchess 
    VERSION 1.1.0
    LANGUAGES CXX
)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check GCC version for C++23 support
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        message(FATAL_ERROR "GCC >= 13 is required for C++23 support. Detected: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# =============================================================================
# Project Options
# =============================================================================
option(BUILD_TESTS "Build standard tests" OFF)
option(BUILD_TESTS_EXTRA "Build extra tests (Perft and Searcht)" OFF)
option(BUILD_EXECUTABLES_EXTRA "Build extra executables (GenSFen and NNUE_Convert)" OFF)
option(ENABLE_LTO "Enable Link Time Optimization for Release builds" ON)
option(ENABLE_STATIC "Enable static compilation" ON)

# Set Release as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =============================================================================
# Compiler Configuration
# =============================================================================
if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(STATUS "Compiler frontend: MSVC-compatible")
    set(COMPILER_OPTIONS
        /W4 /EHsc /arch:AVX2
        $<$<CONFIG:RELEASE>:/O2 /fp:fast /DNDEBUG>
        $<$<CONFIG:RELWITHDEBINFO>:/O2 /fp:fast /DNDEBUG /Zi>
        $<$<CONFIG:DEBUG>:/Od /DDEBUG /Zi>
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND COMPILER_OPTIONS /MP)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
else()
    set(WARNING_FLAGS -Wall -Wextra -Wshadow)
    set(ARCH_FLAGS -march=haswell -mtune=generic)
    set(OPTIMIZATION_FLAGS -O3 -ffast-math -DNDEBUG -fomit-frame-pointer -funroll-loops)
    set(COMPILER_OPTIONS
        ${WARNING_FLAGS} ${ARCH_FLAGS}
        $<$<CONFIG:RELEASE>:${OPTIMIZATION_FLAGS}>
        $<$<CONFIG:RELWITHDEBINFO>:${OPTIMIZATION_FLAGS} -g>
        $<$<CONFIG:DEBUG>:-O0 -DDEBUG -g>
    )
endif()

# =============================================================================
# Linker Configuration
# =============================================================================
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")

    # IPO/LTO Configuration
    if(ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        if(ipo_supported)
            message(STATUS "IPO/LTO enabled")
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(STATUS "IPO/LTO not supported: ${ipo_error}")
        endif()
    endif()

    # Static Compilation
    if(ENABLE_STATIC AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "STATIC linking mode enabled")
        set(LINK_FLAGS ${LINK_FLAGS} -static)
    endif()
    
endif()

# =============================================================================
# Dependencies
# =============================================================================
# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# GoogleTest
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0 # 30-Apr-2025
    )
    set(BUILD_GMOCK OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(googletest)
    
    # Disable LTO for GoogleTest targets
    set_property(TARGET gtest PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
    set_property(TARGET gtest_main PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

# Fathom
add_library(fathom external/fathom/src/tbprobe.c)
set_source_files_properties(
    external/fathom/src/tbprobe.c
    PROPERTIES LANGUAGE CXX
)
target_include_directories(fathom PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/fathom/src>
)

# List of external libraries for linking
set(EXTERNAL_LIBRARIES Threads::Threads fathom)

# =============================================================================
# Engine Library
# =============================================================================
# OBJECT
set(INCLUDE_PATHS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
)

set(ENGINE_SOURCES
    src/Attacks.cpp
    src/BitboardUtils.cpp
    src/Board.cpp
    src/Constants.cpp
    src/Debug.cpp
    src/Evaluation.cpp
    src/Fen.cpp
    src/Hash.cpp
    src/Heuristics.cpp
    src/Interface.cpp
    src/Move.cpp
    src/MoveGenerator.cpp
    src/MoveMaker.cpp
    src/MoveScorer.cpp
    src/NNUE.cpp
    src/PV.cpp
    src/Search.cpp
    src/SearchLimits.cpp
    src/Syzygy.cpp
    src/Uci.cpp
    src/Utils.cpp
    src/ZobristKeys.cpp
)

# Create OBJECT library for main executable (single file distribution)
add_library(engine_obj OBJECT ${ENGINE_SOURCES})
target_include_directories(engine_obj PUBLIC ${INCLUDE_PATHS})
target_compile_options(engine_obj PUBLIC ${COMPILER_OPTIONS})
target_link_libraries(engine_obj PUBLIC ${EXTERNAL_LIBRARIES})

# INTERFACE
add_library(engine INTERFACE)
target_link_libraries(engine INTERFACE engine_obj)

# STATIC
# Create STATIC library for tests and tools (faster compilation)
add_library(engine_static STATIC $<TARGET_OBJECTS:engine_obj>)
target_link_libraries(engine_static PUBLIC engine)

# =============================================================================
# Casanchess Executable
# =============================================================================
add_executable(casanchess Main.cpp)

target_link_libraries(casanchess PRIVATE $<TARGET_OBJECTS:engine_obj> engine)
target_link_options(casanchess PRIVATE ${LINK_FLAGS})

# =============================================================================
# Data Files
# =============================================================================
# Copy NNUE files to build directory
file(GLOB NNUE_FILES "${CMAKE_SOURCE_DIR}/data/*.nnue")
if(NNUE_FILES)
    file(COPY ${NNUE_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    message(STATUS "Copied NNUE files: ${NNUE_FILES}")
else()
    message(WARNING "No NNUE files found in data/ directory")
endif()

# =============================================================================
# Tests
# =============================================================================
if(BUILD_TESTS)
    enable_testing()

    # Main test executable
    add_executable(tests
        tests/test-Main.cpp
        tests/test-Board.cpp
        tests/test-Evaluation.cpp
        tests/test-Hash.cpp
        tests/test-Misc.cpp
        tests/test-SEE.cpp
        tests/test-Utils.cpp
        tests/test-ZobristKey.cpp
    )
    
    target_compile_options(tests PRIVATE ${COMPILER_OPTIONS})
    target_link_libraries(tests PRIVATE engine_static gtest)

    # Extra tests
    if(BUILD_TESTS_EXTRA)
        # Perft test
        add_executable(perft tests/test-Perft.cpp)
        target_compile_options(perft PRIVATE ${COMPILER_OPTIONS})
        target_link_libraries(perft PRIVATE engine_static gtest)

        # Search test
        add_executable(searcht tests/test-Search.cpp)
        target_compile_options(searcht PRIVATE ${COMPILER_OPTIONS})
        target_link_libraries(searcht PRIVATE engine_static gtest)
    endif()
endif()

# =============================================================================
# Extra Executables
# =============================================================================
if(BUILD_EXECUTABLES_EXTRA)
    # GenSFen executable
    add_executable(gensfen
        src/gensfen/GenSFen.cpp
        src/gensfen/Main.cpp
    )
    target_compile_options(gensfen PRIVATE ${COMPILER_OPTIONS})
    target_link_libraries(gensfen PRIVATE engine_static)

    # NNUE_Convert executable
    add_executable(nnue_convert
        src/nnue_convert/NNUE_Convert.cpp
    )
    target_compile_options(nnue_convert PRIVATE ${COMPILER_OPTIONS})
    target_link_libraries(nnue_convert PRIVATE engine_static)
endif()
