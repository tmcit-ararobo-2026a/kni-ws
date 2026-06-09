cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(encoder STATIC
    src/encoder.cpp

)

target_include_directories(encoder PUBLIC
    include
)

target_link_libraries(encoder PRIVATE stm32cubemx)