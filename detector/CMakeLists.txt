cmake_minimum_required(VERSION 3.16.3)
project(detector)

# Подгрузить FFmpeg
if (NOT FFMPEG_LIB_DIR)
    message(FATAL_ERROR "You must specify FFMPEG_LIB_DIR")
endif()
if (NOT FFMPEG_INCLUDES_DIR)
    message(FATAL_ERROR "You must specify FFMPEG_INCLUDES_DIR")
endif()

# Подгрузить OpenCV
#   1. Через OpenCV_DIR
#   2. Через OpenCV_LIB_DIR, OpenCV_INCLUDES_DIR
if (NOT OpenCV_DIR AND NOT OpenCV_LIB_DIR AND NOT OpenCV_INCLUDES_DIR)
    message(FATAL_ERROR 
        "You must specify either OpenCV_DIR or these two: OpenCV_LIB_DIR, OpenCV_INCLUDES_DIR")
endif()
if (OpenCV_DIR)
    find_package(OpenCV COMPONENTS core imgproc dnn)
    set(OpenCV_INCLUDES_DIR ${OpenCV_INCLUDE_DIRS})
endif()

add_executable(
    ${PROJECT_NAME} 
    detector.cpp 
    math.h 
    ffmpeg.h
    nn_filter.h
)

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
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDES_DIR})

target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_DIR})
if (OpenCV_LIB_DIR)
    target_link_directories(${PROJECT_NAME} PRIVATE ${OpenCV_LIB_DIR})
endif()

target_link_libraries(
    ${PROJECT_NAME} 
    PRIVATE 
    avcodec 
    avformat 
    avutil 
    opencv_core
    opencv_imgproc
    opencv_dnn
)

# Скопировать файл NN-модели
add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/nn_filter.onnx
            ${CMAKE_CURRENT_BINARY_DIR}/nn_filter.onnx
)
