#ifndef APP_engine_GUI_H
#define APP_engine_GUI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
#define ANIEND                      \
    while (lv_anim_count_running()) \
        lv_task_handler(); //等待动画完成

    void engine_gui_init(void);
    void display_engine(const char *now);
    void engine_gui_del(void);
    void engine_obj_del(void);
    void engine_style_del(void);
    void display_engine_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_engine;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif