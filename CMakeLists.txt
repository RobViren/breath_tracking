cmake_minimum_required(VERSION 2.6)
project (breath_tracking)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
find_package(OpenCV REQUIRED)

include_directories(include)

file(GLOB PROJECT_SRCS 	
	${PROJECT_SOURCE_DIR}/src/*.cpp
	${PROJECT_SOURCE_DIR}/include/*.h
)

add_executable(${PROJECT_NAME} ${PROJECT_SRCS})

if(OpenCV_FOUND)
	include_directories(${OpenCV_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
endif()