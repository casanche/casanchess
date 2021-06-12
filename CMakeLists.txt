cmake_minimum_required(VERSION 2.6)
project(Casanchess CXX)

set(CMAKE_CXX_STANDARD 20)

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wshadow -march=native -pedantic") #-Ofast -fprofile-generate=profile -Wextra
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG -flto")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -g")
	set(LINK_LIBRARIES "pthread")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /W3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG /GL /fp:fast /Ob3") #/arch:AVX /fp:fast /Ob3 /clr
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
	#SET(GTEST_ROOT "C:/Program Files/gtest")
endif()

#Library
include_directories(./include)
file(GLOB SOURCES src/*.cpp)
list(REMOVE_ITEM SOURCES src/Main.cpp)
add_library(engine STATIC ${SOURCES})

add_library(tests-common STATIC tests/test-Common.cpp)

#Main
add_executable(casanchess Main.cpp)
target_link_libraries(casanchess engine ${LINK_LIBRARIES}) #CMAKE_LINK_LIBRARY_FLAG

set(BUILD_TESTS 1)
if(BUILD_TESTS)
	#Google Test
	find_package(GTest REQUIRED)
	include_directories(${GTEST_INCLUDE_DIRS})
	set(TEST_LIBRARIES ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} tests-common engine ${LINK_LIBRARIES})
	set(TEST_SOURCES
		tests/test-Board.cpp
		tests/test-Misc.cpp
		tests/test-Utils.cpp
		tests/test-ZobristKey.cpp
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
