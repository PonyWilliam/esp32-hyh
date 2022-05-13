#include "engine_gui.h"

#include "driver/lv_port_indev.h"
#include "lvgl.h"
LV_FONT_DECLARE(ch_font20);
lv_obj_t *engine_gui = NULL;

lv_obj_t *title = NULL;
lv_obj_t *tips = NULL;
lv_obj_t *status = NULL;
const char *now;
lv_style_t default_style;
lv_style_t title_style;
lv_style_t label_style;
lv_style_t info_style;
lv_style_t ch_style;

void engine_gui_init(void)
{ 
    lv_style_init(&default_style);
    //初始化样式表

    lv_style_init(&title_style);
    lv_style_set_text_opa(&title_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_color(&title_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&title_style, LV_STATE_DEFAULT, &lv_font_montserrat_30);

    lv_style_init(&label_style);
    lv_style_set_text_opa(&label_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_24);

    lv_style_init(&info_style);
    lv_style_set_text_opa(&info_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_color(&info_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&info_style, LV_STATE_DEFAULT, &lv_font_montserrat_20);

    lv_style_set_bg_color(&default_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_bg_color(&default_style, LV_STATE_PRESSED, LV_COLOR_GRAY);
    lv_style_set_bg_color(&default_style, LV_STATE_FOCUSED, LV_COLOR_BLACK);
    lv_style_set_bg_color(&default_style, LV_STATE_FOCUSED | LV_STATE_PRESSED, lv_color_hex(0xf88));


    lv_style_init(&ch_style);
    lv_style_set_text_opa(&ch_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_color(&ch_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&ch_style, LV_STATE_DEFAULT, &ch_font20);

    //将标签创建，提示左右移动可以切换开关状态


}

/*
 * 其他函数请根据需要添加
 */

void display_engine_init(){
    //初始化display init
    lv_obj_t *act_obj = lv_scr_act();
    if (act_obj == engine_gui)
        return;//初始化完毕，不需要创建对象
    //没有act，创建对象
    engine_gui_del();
    engine_gui = lv_obj_create(NULL, NULL);//主控制区
    lv_obj_add_style(engine_gui, LV_BTN_PART_MAIN, &default_style);
    if (NULL != title)
    {
        lv_obj_clean(title);
        lv_obj_clean(tips);
        lv_obj_clean(status);
    }
    lv_obj_clean(act_obj); // 清空此前页面
    title = lv_label_create(engine_gui, NULL);
    tips = lv_label_create(engine_gui, NULL);
    status = lv_label_create(engine_gui, NULL);
}
void display_engine(const char *now)
{
    display_engine_init();
    lv_obj_add_style(title, LV_LABEL_PART_MAIN, &title_style);
    lv_label_set_recolor(title, true); //文本重绘色开启
    lv_label_set_text(title, "#ff0000 Smart\t\tControl");//绘色写入
    lv_obj_align(title, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, -220);

    lv_obj_add_style(status, LV_LABEL_PART_MAIN, &label_style);
    lv_label_set_recolor(status, true); //文本重绘色开启
    lv_label_set_text_fmt(status, "%s",now);
    lv_obj_align(status, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, -140);

    lv_obj_add_style(tips, LV_LABEL_PART_MAIN, &ch_style);
    lv_label_set_recolor(tips, true); //文本重绘色开启
    lv_label_set_text(tips, "left->关，right->开");
    lv_obj_align(tips, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, -40);

    lv_scr_load(engine_gui);
}

void engine_obj_del(void){
    if(NULL != title){
        lv_obj_clean(title);
        lv_obj_clean(status);
        lv_obj_clean(tips);
        title = NULL;
        status = NULL;
        tips = NULL;
    }
    engine_style_del();
}
void engine_style_del(void){
    lv_style_reset(&default_style);
    lv_style_reset(&title_style);
    lv_style_reset(&label_style);
    lv_style_reset(&info_style);
    lv_style_reset(&ch_style);
}
void engine_gui_del(void)
{
    //手动释放内存，防止内存泄漏
    engine_obj_del();
    if (NULL != engine_gui)
    {
        lv_obj_clean(engine_gui);
        engine_gui = NULL;
    }
    
}