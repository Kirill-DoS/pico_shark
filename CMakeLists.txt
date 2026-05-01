cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
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
    src/onewire_library.cpp
    src/enkoder.cpp
    src/ds18b20.cpp
)

pico_enable_stdio_usb(pico_shark 1)

pico_generate_pio_header(pico_shark ${CMAKE_CURRENT_LIST_DIR}/src/pio/onewire_library.pio)
pico_generate_pio_header(pico_shark ${CMAKE_CURRENT_LIST_DIR}/src/pio/DS18B20.pio)

target_link_libraries(pico_shark PRIVATE
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_dma
    hardware_spi
    lvgl
    ui
    #"-Wl,--whole-archive ui -Wl,--no-whole-archive"
)

target_compile_options(pico_shark PRIVATE -O2 -g -Wall)
pico_add_extra_outputs(pico_shark)
