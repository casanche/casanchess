cmake_minimum_required(VERSION 3.14)
project(Casanchess CXX)

set(CMAKE_CXX_STANDARD 20)

## Options
option(BUILD_TESTS "Build standard tests" ON)
option(BUILD_TESTS_EXTRA "Build extra tests (Perft and Searcht)" OFF)
option(BUILD_EXECUTABLES_EXTRA "Build extra executables (GenSFen and NNUE_Convert)" OFF)

if(BUILD_TESTS)
	## Download dependency: GoogleTest
	include(FetchContent)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG        release-1.11.0 # 21-Jun-2021
	)
	## For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	set(BUILD_GMOCK OFF CACHE INTERNAL "")
	FetchContent_MakeAvailable(googletest)
endif()

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wshadow -march=native -pedantic") #-Ofast -fprofile-generate=profile -Wextra
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG -flto=auto")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -g")
	set(LINK_LIBRARIES "pthread")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /W3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG /GL /fp:fast /Ob3") #/arch:AVX /fp:fast /Ob3 /clr
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
endif()

## Library
include_directories(./include)
file(GLOB SOURCES src/*.cpp)
list(REMOVE_ITEM SOURCES src/Main.cpp)
add_library(engine STATIC ${SOURCES})

## Main executable
add_executable(casanchess Main.cpp)
target_link_libraries(casanchess engine ${LINK_LIBRARIES}) #CMAKE_LINK_LIBRARY_FLAG

## Copy .nnue file to build/ folder
add_custom_command(
	TARGET casanchess POST_BUILD
	COMMAND $(CMAKE_COMMAND) -E copy $(CMAKE_SOURCE_DIR)/data/*.nnue .
)

if(BUILD_EXECUTABLES_EXTRA)
	## GenSFen
	file(GLOB GENSFEN_SOURCES src/gensfen/*.cpp)
	add_executable(gensfen ${GENSFEN_SOURCES})
	target_link_libraries(gensfen engine ${LINK_LIBRARIES})

	## NNUE_Convert
	file(GLOB NNUE_CONVERT_SOURCES src/nnue_convert/*.cpp)
	add_executable(nnue_convert ${NNUE_CONVERT_SOURCES})
	target_link_libraries(nnue_convert engine ${LINK_LIBRARIES})
endif()

if(BUILD_TESTS)
	enable_testing()

	set(TEST_LIBRARIES engine gtest) #gtest_main
	set(TEST_SOURCES
		tests/test-Main.cpp
		tests/test-Board.cpp
		tests/test-Misc.cpp
		tests/test-Utils.cpp
		tests/test-ZobristKey.cpp
	)
	add_executable(tests ${TEST_SOURCES})
	target_link_libraries(tests ${TEST_LIBRARIES})

	#include(GoogleTest)
	#gtest_discover_tests(tests)

	if(BUILD_TESTS_EXTRA)
		## Perft test
		add_executable(perft tests/test-Perft.cpp)
		target_link_libraries(perft ${TEST_LIBRARIES})

		## Search test
		add_executable(searcht tests/test-Search.cpp)
		target_link_libraries(searcht ${TEST_LIBRARIES})

		## Suites
		#add_executable(wac tests/suite-WAC.cpp)
		#target_link_libraries(wac ${TEST_LIBRARIES})
	endif()
endif()
