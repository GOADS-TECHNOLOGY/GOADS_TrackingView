cmake_minimum_required(VERSION 3.10)

project(goadsodtrack VERSION 1.0.0)

if (ENABLE_ASAN)
	message(STATUS "BUILD WITH ADDRESS SANITIZER")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)

set(RKNN_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/runtime/RK356X/Linux/librknn_api)
set(RKNN_RT_LIB ${RKNN_API_PATH}/aarch64/librknnrt.so)
include_directories(${RKNN_API_PATH}/include)
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

find_package(OpenCV REQUIRED)
find_package(cppzmq)

message(${OpenCV_INCLUDE_DIRS})

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB RKNN_HEADER_FILES ${RKNN_API_PATH}/include/*.h)
file(GLOB UTILS_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(/usr/local/include/eigen3)

add_library(${PROJECT_NAME} SHARED ${SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/include/eigen3
    ${LIBRKNNRT_INCLUDES}
    ${cppzmq_INCLUDE_DIR}
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

add_executable(${PROJECT_NAME}_test
    main.cpp
)

target_link_libraries(${PROJECT_NAME}_test
    ${PROJECT_NAME}
)

# Webcam test
add_executable(webcam_test
    webcam_test.cpp
)

target_link_libraries(webcam_test
    ${OpenCV_LIBS}
)

# ZMQ test
add_executable(subcrise_zmq_test
    subcrise_zmq.cpp
)

target_link_libraries(subcrise_zmq_test
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

set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "/usr/local/lib")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include/${PROJECT_NAME})
install(FILES ${RKNN_HEADER_FILES} DESTINATION include/rknn)
install(FILES ${UTILS_HEADER_FILES} DESTINATION include/utils)
install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/bus.jpg DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/coco_80_labels_list.txt DESTINATION model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)