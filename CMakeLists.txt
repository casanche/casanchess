cmake_minimum_required(VERSION 2.6)
project(Casanchess CXX)

set(CMAKE_CXX_STANDARD 17)

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wshadow -march=native -pedantic") #-Ofast
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG -flto")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -g")
endif(UNIX)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG")
endif(MSVC)

#Library
include_directories(./include)
file(GLOB SOURCES src/*.cpp)
list(REMOVE_ITEM SOURCES src/Main.cpp)
add_library(engine STATIC ${SOURCES})

add_library(tests-common STATIC tests/test-Common.cpp)

#Main
add_executable(casanchess Main.cpp)
target_link_libraries(casanchess engine)

set(BUILD_TESTS 1)
if(BUILD_TESTS)
	#Google Test
	find_package(GTest REQUIRED)
	include_directories(${GTEST_INCLUDE_DIRS})
	set(TEST_LIBRARIES ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread tests-common engine)
	set(TEST_SOURCES
		tests/test-BitboardUtils.cpp
		tests/test-Board.cpp
		tests/test-ZobristKey.cpp
		tests/test-Misc.cpp
	)
	add_executable(tests ${TEST_SOURCES})
	target_link_libraries(tests ${TEST_LIBRARIES})

	#Perft test
	add_executable(perft tests/test-Perft.cpp)
	target_link_libraries(perft ${TEST_LIBRARIES})

	#Search test
	add_executable(searcht tests/test-Search.cpp)
	target_link_libraries(searcht ${TEST_LIBRARIES})

	#Suites
	add_executable(wac tests/suite-WAC.cpp)
	target_link_libraries(wac ${TEST_LIBRARIES})
endif()
