cmake_minimum_required(VERSION 3.0.0)
project(HelloCalib VERSION 0.1.0)

include(CTest)
enable_testing()



find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR} )

set(SOURCES main.cpp Settings.h )

add_executable(HelloCalib ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


set(EXT_LIBS opencv_core opencv_imgcodecs opencv_highgui opencv_imgproc opencv_calib3d opencv_features2d)
target_link_libraries(HelloCalib ${EXT_LIBS})
set(CMAKE_CXX_STANDARD 17)
target_compile_features(HelloCalib PRIVATE cxx_std_17) # https://stackoverflow.com/a/53830867