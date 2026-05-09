SET(SOURCES screens/ui_MainScreen.c
    screens/ui_SensorsScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_cyberpank_bg_png.c)

add_library(ui ${SOURCES})
