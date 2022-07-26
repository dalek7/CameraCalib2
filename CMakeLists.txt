cmake_minimum_required(VERSION 3.0.0)
project(HelloCalib VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(HelloCalib main.cpp)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR} )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


set(EXT_LIBS opencv_core opencv_imgcodecs opencv_highgui opencv_imgproc opencv_calib3d opencv_features2d)
target_link_libraries(HelloCalib ${EXT_LIBS})
