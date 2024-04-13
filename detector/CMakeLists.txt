cmake_minimum_required(VERSION 3.16.3)
project(detector)

if (NOT FFMPEG_LIB_DIR)
	message(FATAL_ERROR "You must specify FFMPEG_LIB_DIR")
endif()
if (NOT FFMPEG_INCLUDES_DIR)
	message(FATAL_ERROR "You must specify FFMPEG_INCLUDES_DIR")
endif()

add_executable(${PROJECT_NAME} detector.cpp math.h ffmpeg.h)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

if (UNIX AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Для тщательной отладки
    target_compile_options(
        ${PROJECT_NAME} PRIVATE 
        -Wall -Werror -Wextra -Wpedantic 
        -Wno-error=deprecated-declarations -Wdeprecated-declarations
        -Wno-error=unused-function -Wno-unused-function
        -Wno-error=unused-variable -Wno-unused-variable
        -Wno-error=unused-but-set-variable -Wno-unused-but-set-variable 
        -Wno-error=unused-const-variable -Wno-unused-const-variable
        -Wno-error=unused-parameter -Wno-unused-parameter
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDES_DIR})

target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_DIR})
target_link_libraries(
	${PROJECT_NAME} 
	PRIVATE 
	avcodec 
	avformat 
	avutil 
)
