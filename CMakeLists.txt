cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350)

# ... (инициализация pico и прочее)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(pico_shark C CXX ASM)
pico_sdk_init()

# --- ГЛОБАЛЬНЫЕ НАСТРОЙКИ ---
# 1. Добавляем корень проекта в пути поиска для ВСЕХ файлов
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 2. Добавляем дефайн для ВСЕХ файлов, чтобы LVGL искал lv_conf.h просто по имени
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)
# ----------------------------

# Сначала добавляем библиотеки
add_subdirectory(lvgl)
add_subdirectory(ui)

target_include_directories(ui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
)

target_compile_definitions(ui PUBLIC LV_CONF_INCLUDE_SIMPLE=1)

target_link_libraries(ui PUBLIC lvgl)

# Теперь описываем основной проект
add_executable(pico_shark
    main.cpp
    ST7789/PicoGFX.cpp
    ST7789/ST7789.cpp
    setup.cpp
    src/enkoder/enkoder.cpp
    src/temp_press/temp_press.cpp
    src/pn532/pn532.cpp
)

pico_enable_stdio_usb(pico_shark 1)
pico_enable_stdio_uart(pico_shark 0)
# generate pio header file
# pico_generate_pio_header(nec_receive_library ${CMAKE_CURRENT_LIST_DIR}/nec_receive.pio)
# pico_generate_pio_header(nec_receive_library ${CMAKE_CURRENT_LIST_DIR}/nec_receive.pio)
# pico_generate_pio_header(nec_transmit_library ${CMAKE_CURRENT_LIST_DIR}/nec_carrier_burst.pio)
# pico_generate_pio_header(nec_transmit_library ${CMAKE_CURRENT_LIST_DIR}/nec_carrier_control.pio)


target_link_libraries(pico_shark PRIVATE
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_dma
    hardware_spi
    hardware_i2c
    hardware_uart
    lvgl
    ui
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_sntp
    #"-Wl,--whole-archive ui -Wl,--no-whole-archive"
)

target_compile_options(pico_shark PRIVATE -O2 -g -Wall)
pico_add_extra_outputs(pico_shark)
