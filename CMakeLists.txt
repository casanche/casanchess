cmake_minimum_required(VERSION 3.20)
project(Casanchess
    VERSION 1.1.2
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
option(BUILD_NNUE_TOOLS "Build NNUE tools (NNUE_Datagen and NNUE_Convert)" OFF)
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
    set(ARCH_FLAGS /arch:AVX2)
    set(COMPILER_OPTIONS
        /W4 /EHsc ${ARCH_FLAGS}
        $<$<CONFIG:RELEASE>:/O2 /fp:fast /DNDEBUG>
        $<$<CONFIG:RELWITHDEBINFO>:/O2 /fp:fast /DNDEBUG /Zi>
        $<$<CONFIG:DEBUG>:/Od /DDEBUG /Zi>
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND COMPILER_OPTIONS /MP)
    endif()
    if(ENABLE_STATIC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
    endif()
else()
    set(WARNING_FLAGS -Wall -Wextra -Wshadow)
    set(ARCH_FLAGS -march=x86-64-v3)
    set(OPTIMIZATION_FLAGS -O3 -ffast-math -DNDEBUG -funroll-loops)
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
# IPO/LTO Configuration
if(ENABLE_LTO)
    # Forcing FullLTO. CMake defaults to ThinLTO for Clang
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_COMPILE_OPTIONS_IPO "-flto=full")
    endif()

    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        message(STATUS "IPO/LTO enabled for Release builds")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    elseif(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo)$")
        message(FATAL_ERROR "LTO requested but not supported: ${ipo_error}")
    else()
        message(STATUS "IPO/LTO not supported: ${ipo_error}")
    endif()
endif()

if(ENABLE_STATIC AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND
   (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MINGW))
    message(STATUS "STATIC linking mode enabled (${CMAKE_CXX_COMPILER_ID} on ${CMAKE_SYSTEM_NAME})")
    add_link_options(-static)
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
        GIT_TAG        v1.18.0 # 10-Aug-2026
        GIT_SHALLOW    TRUE
    )
    set(BUILD_GMOCK OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(googletest)

    # Disable LTO for GoogleTest targets
    set_target_properties(gtest gtest_main PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE FALSE
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO FALSE
    )
endif()

# Fathom
add_library(fathom external/fathom/src/tbprobe.c)
set_source_files_properties(
    external/fathom/src/tbprobe.c
    PROPERTIES LANGUAGE CXX
)
target_compile_options(fathom PRIVATE ${ARCH_FLAGS})
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
    src/Ambition.cpp
    src/Attacks.cpp
    src/BitboardUtils.cpp
    src/Board.cpp
    src/Constants.cpp
    src/Debug.cpp
    src/Engine.cpp
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

# =============================================================================
# Data Files
# =============================================================================
file(GLOB NNUE_FILE CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/*.nnue")
get_filename_component(NNUE_FILENAME "${NNUE_FILE}" NAME)

# Make the embedding source change when the network changes
file(SHA256 "${NNUE_FILE}" NNUE_SHA256)

# Copy the NNUE file for tests and other executables
configure_file(
    "${NNUE_FILE}"
    "${CMAKE_CURRENT_BINARY_DIR}/${NNUE_FILENAME}"
    COPYONLY
)

target_compile_definitions(engine_obj PUBLIC NNUE_FILENAME="${NNUE_FILENAME}")

if(WIN32)
    enable_language(RC)
    set(NNUE_EMBED_EXTENSION rc)
    set(NNUE_EMBED_SOURCE src/nnue_embed/EmbeddedNetworkWindows.cpp)
else()
    enable_language(ASM)
    set(NNUE_EMBED_EXTENSION S)
    set(NNUE_EMBED_SOURCE src/nnue_embed/EmbeddedNetworkLinux.cpp)
endif()

configure_file(
    src/nnue_embed/EmbeddedNetwork.${NNUE_EMBED_EXTENSION}.in
    EmbeddedNetwork.${NNUE_EMBED_EXTENSION}
    @ONLY
)

target_sources(casanchess PRIVATE
    ${NNUE_EMBED_SOURCE}
    "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedNetwork.${NNUE_EMBED_EXTENSION}"
)

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

    target_link_libraries(tests PRIVATE engine_static gtest)

    # Extra tests
    if(BUILD_TESTS_EXTRA)
        # Perft test
        add_executable(perft tests/test-Perft.cpp)
        target_link_libraries(perft PRIVATE engine_static gtest)

        # Search test
        add_executable(searcht tests/test-Search.cpp)
        target_link_libraries(searcht PRIVATE engine_static gtest)
    endif()
endif()

# =============================================================================
# Extra Executables
# =============================================================================
if(BUILD_NNUE_TOOLS)
    # nnue-datagen executable
    add_executable(nnue-datagen
        src/nnue_datagen/Main.cpp
        src/nnue_datagen/Datagen.cpp
        src/nnue_datagen/RandomPosition.cpp
    )
    target_link_libraries(nnue-datagen PRIVATE engine_static)

    # nnue-convert executable
    add_executable(nnue-convert
        src/nnue_convert/Main.cpp
        src/nnue_convert/Convert.cpp
    )
    target_link_libraries(nnue-convert PRIVATE engine_static)
endif()
