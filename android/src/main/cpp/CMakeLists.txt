cmake_minimum_required(VERSION 3.10)
project("biometric_sdk")

# 1. OpenCV ka rasta (path) batayein
#set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencv/native/jni)
set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/native/jni)
find_package(OpenCV REQUIRED)

# 2. Aapki C++ file jisme humne image enhancement ka code likha hai
add_library(
        native-lib
        SHARED
        native-lib.cpp
)

# 3. Android log library dhundein (debugging ke liye)
find_library(log-lib log)

# 4. OpenCV aur Log library ko aapke C++ code ke sath link karein
target_link_libraries(
        native-lib
        ${OpenCV_LIBS}
        ${log-lib}
)
