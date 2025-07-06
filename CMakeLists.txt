cmake_minimum_required(VERSION 3.14)
project(Casanchess 
    VERSION 0.9.0
    LANGUAGES CXX
)

# =============================================================================
# Project Options
# =============================================================================
option(BUILD_TESTS "Build standard tests" OFF)
option(BUILD_TESTS_EXTRA "Build extra tests (Perft and Searcht)" OFF)
option(BUILD_EXECUTABLES_EXTRA "Build extra executables (GenSFen and NNUE_Convert)" OFF)
option(ENABLE_LTO "Enable Link Time Optimization for Release builds" ON)

# Set Release as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =============================================================================
# Compiler Configuration
# =============================================================================
if(MSVC)
    message(STATUS "Compiler: MSVC")
    set(COMPILER_OPTIONS
        /W4 /arch:AVX2
        $<$<CONFIG:RELEASE>:/O2 /GL /fp:fast /DNDEBUG>
        $<$<CONFIG:RELWITHDEBINFO>:/O2 /GL /fp:fast /DNDEBUG /Zi>
        $<$<CONFIG:DEBUG>:/Od /DDEBUG /Zi>
    )
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

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# IPO/LTO Configuration
if(ENABLE_LTO AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        message(STATUS "IPO/LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: ${ipo_error}")
    endif()
endif()

# Windows-specific configuration
if(WIN32)
    if(NOT MSVC)
        message(STATUS "Set linking mode -- STATIC")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
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

# =============================================================================
# Engine Library
# =============================================================================
set(ENGINE_SOURCES
    src/Attacks.cpp
    src/BitboardUtils.cpp
    src/Board.cpp
    src/Evaluation.cpp
    src/Fen.cpp
    src/Hash.cpp
    src/Heuristics.cpp
    src/Interface.cpp
    src/Move.cpp
    src/MoveGenerator.cpp
    src/MoveMaker.cpp
    src/NNUE.cpp
    src/Search.cpp
    src/Uci.cpp
    src/Utils.cpp
    src/ZobristKeys.cpp
)

# Create OBJECT library for main executable (single file distribution)
add_library(engine OBJECT ${ENGINE_SOURCES})
# Create STATIC library for tests and tools (faster compilation)
add_library(engine_static STATIC ${ENGINE_SOURCES})

# Set include directories for both libraries
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_include_directories(engine_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link dependencies
target_link_libraries(engine PUBLIC Threads::Threads)
target_link_libraries(engine_static PUBLIC Threads::Threads)

# Apply compiler configuration to both libraries
target_compile_options(engine PRIVATE ${COMPILER_OPTIONS})
target_compile_options(engine_static PRIVATE ${COMPILER_OPTIONS})

# =============================================================================
# Casanchess Executable
# =============================================================================
add_executable(casanchess Main.cpp)

target_link_libraries(casanchess PRIVATE $<TARGET_OBJECTS:engine>)
target_include_directories(casanchess PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(casanchess PRIVATE ${COMPILER_OPTIONS})

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
# Extra Executables
# =============================================================================
if(BUILD_EXECUTABLES_EXTRA)
    # GenSFen executable
    add_executable(gensfen
        src/gensfen/GenSFen.cpp
        src/gensfen/Main.cpp
    )
    target_link_libraries(gensfen PRIVATE engine_static)
    target_compile_options(gensfen PRIVATE ${COMPILER_OPTIONS})

    # NNUE_Convert executable
    add_executable(nnue_convert
        src/nnue_convert/NNUE_Convert.cpp
    )
    target_link_libraries(nnue_convert PRIVATE engine_static)
    target_compile_options(nnue_convert PRIVATE ${COMPILER_OPTIONS})
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
        tests/test-Misc.cpp
        tests/test-Utils.cpp
        tests/test-ZobristKey.cpp
    )
    
    target_link_libraries(tests PRIVATE engine_static gtest)
    target_compile_options(tests PRIVATE ${COMPILER_OPTIONS})

    # Extra tests
    if(BUILD_TESTS_EXTRA)
        # Perft test
        add_executable(perft tests/test-Perft.cpp)
        target_link_libraries(perft PRIVATE engine_static gtest)
        target_compile_options(perft PRIVATE ${COMPILER_OPTIONS})

        # Search test
        add_executable(searcht tests/test-Search.cpp)
        target_link_libraries(searcht PRIVATE engine_static gtest)
        target_compile_options(searcht PRIVATE ${COMPILER_OPTIONS})
    endif()
endif()
