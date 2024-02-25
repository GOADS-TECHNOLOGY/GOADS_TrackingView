cmake_minimum_required(VERSION 3.10)

project(goads_trackview VERSION 1.0.0)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)

set(RKNN_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/runtime/RK356X/Linux/librknn_api)
set(RKNN_RT_LIB ${RKNN_API_PATH}/aarch64/librknnrt.so)
include_directories(${RKNN_API_PATH}/include)
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

find_package(OpenCV REQUIRED)

find_package(cppzmq)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(/usr/local/include/eigen3)
include_directories(${cppzmq_INCLUDE_DIR})

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(${PROJECT_NAME}
    main.cpp
    src/postprocess.cpp
    src/yolov8.cpp
    src/bytetracker.cpp
    src/kalmanfilter.cpp
    src/strack.cpp
    src/lapjv.cpp
    src/utils.cpp
)

target_link_libraries(${PROJECT_NAME}
    fileutils
    imageutils
    imagedrawing
    ${RKNN_RT_LIB}    
    ${LIBRKNNRT}
    ${OpenCV_LIBS}
    cppzmq
)

# Webcam test
add_executable(webcam_test
    webcam_test.cpp
)

target_link_libraries(webcam_test
    ${OpenCV_LIBS}
)

# ZMQ test
add_executable(zmq_test
    subcrise_zmq.cpp
)

target_link_libraries(zmq_test
    cppzmq
)

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(${PROJECT_NAME}
    log
)
endif()

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBRKNNRT_INCLUDES}
    ${cppzmq_INCLUDE_DIR}
)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(PROGRAMS ${RKNN_RT_LIB} DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/bus.jpg DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/coco_80_labels_list.txt DESTINATION model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)