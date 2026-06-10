cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. ライブラリの定義（ソースファイルの追加）
add_library(encoder STATIC
    src/encoder.cpp
    src/vesc_can.cpp
    src/fdcan_driver.cpp
    ${CMAKE_SOURCE_DIR}/gn10-can/drivers/stm32_fdcan/driver_stm32_fdcan.cpp
    
)

# 2. インクルードパスの通し方（ハイフンに修正し、/include まで指定）
target_include_directories(encoder PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/gn10-can          # drivers/... 用

)
target_link_libraries(encoder PRIVATE stm32cubemx)
target_link_libraries(encoder PRIVATE USBX)
target_link_libraries(encoder PUBLIC gn10_can)
