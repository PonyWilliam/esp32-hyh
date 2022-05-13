#include "engine.h"
#include "engine_gui.h"
#include "sys/app_controller.h"
#include "common.h"
#include <HTTPClient.h>


#define ENGINE_APP_NAME "Engine"
const char *res_url,*key,*name;
// 动态数据，APP的生命周期结束也需要释放它
struct EngineAppRunData
{
    int my_switch;
};

// 常驻数据，可以不随APP的生命周期而释放或删除
struct EngineAppForeverData
{
    int my_switch;
};

// 保存APP运行时的参数信息，理论上关闭APP时推荐在 xxx_exit_callback 中释放掉
static EngineAppRunData *run_data = NULL;

// 当然你也可以添加恒定在内存中的少量变量（退出时不用释放，实现第二次启动时可以读取）
// 考虑到所有的APP公用内存，尽量减少 forever_data 的数据占用
static EngineAppForeverData forever_data;

static int engine_init(void)
{
    res_url = "https://switchiot.dadiqq.cn";//指定服务端URL地址
    key = "a1J4LfeKtKw";//key
    name = "Switch1";//name通过阿里云获取
    // 初始化运行时的参数
    engine_gui_init();
    // 初始化运行时参数
    run_data = (EngineAppRunData *)calloc(1, sizeof(EngineAppRunData));
    run_data->my_switch = 0;
    // 使用 forever_data 中的变量，任何函数都可以用

    display_engine("off");

}

static void engine_process(AppController *sys,
                            const ImuAction *act_info)
{
    if (RETURN == act_info->active)
    {
        sys->app_exit(); // 退出APP
        return;
    }
    if (TURN_LEFT == act_info->active){
        //关
        HTTPClient http;
        run_data -> my_switch = 0;
        //请求开始
        display_engine("request...");
        char *my_url = new char[100];
        sprintf(my_url,"%s/set/%s?name=%s&val=0",res_url,key,name);
        // http.begin("https://switchiot.dadiqq.cn/set/a1J4LfeKtKw?name=Switch1&val=0");
        http.begin(my_url);
        int httpCode = http.GET();
        delay(30);//防止过快刷新屏幕
        //根据请求结果绘制屏幕
        if(httpCode > 0){
            display_engine("Now status:off");
        }else{
            display_engine("error in http request");
        }
        http.end();
        return;
    }
    if (TURN_RIGHT == act_info->active){
        //开
        HTTPClient http;
        display_engine("request...");
        run_data -> my_switch = 1;
        char *my_url = new char[100];
        sprintf(my_url,"%s/set/%s?name=%s&val=1",res_url,key,name);
        // http.begin("https://switchiot.dadiqq.cn/set/a1J4LfeKtKw?name=Switch1&val=1");
        http.begin(my_url);
        int httpCode = http.GET();
        delay(30);//防止过快刷新屏幕
        //根据请求结果绘制屏幕
        if(httpCode > 0){
            display_engine("Now status:on");
        }else{
            display_engine("error in http request");
        }
        http.end();
        return;
    }
}

static int engine_exit_callback(void *param)
{
    // 释放资源
    free(run_data);
    run_data = NULL;
}

static void engine_message_handle(const char *from, const char *to,
                                   APP_MESSAGE_TYPE type, void *message,
                                   void *ext_info)
{
    // 目前主要是wifi开关类事件（用于功耗控制）
    switch (type)
    {
    case APP_MESSAGE_WIFI_CONN:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_AP:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_ALIVE:
    {
        // wifi心跳维持的响应 可以不做任何处理
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
    }
    break;
    default:
        break;
    }
}

APP_OBJ engine_app = {ENGINE_APP_NAME, &app_engine, "Author TJW\nVersion 1.0.0\n",
                       engine_init, engine_process,
                       engine_exit_callback, engine_message_handle};
