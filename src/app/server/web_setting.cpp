
// 参考代码 https://github.com/G6EJD/ESP32-8266-File-Upload

#include "network.h"
#include "common.h"
#include "server.h"
#include "web_setting.h"
#include "FS.h"
#include "HardwareSerial.h"
#include <esp32-hal.h>

boolean sd_present = true;
String webpage = "";
String webpage_header = "";
String webpage_footer = "";

void Send_HTML(const String &content)
{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.send(200, "text/html", "");
    server.sendContent(webpage_header);

    server.sendContent(content);
    server.sendContent(webpage_footer);

    server.sendContent("");
    server.client().stop(); // Stop is needed because no content length was sent
}

String file_size(int bytes)
{
    String fsize = "";
    if (bytes < 1024)
        fsize = String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        fsize = String(bytes / 1024.0, 3) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    else
        fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
    return fsize;
}

#define SETING_CSS ".input {display: block;margin-top: 10px;}"                                          \
                   ".input span {width: 300px;float: left;float: left;height: 36px;line-height: 36px;}" \
                   ".input input {height: 30px;width: 200px;}"                                          \
                   ".input .radio {height: 30px;width: 50px;}"                                          \
                   ".btn {width: 120px;height: 35px;background-color: #000000;border: 0px;color: #ffffff;margin-top: 15px;margin-left: auto;}" // margin-left: 100px;

#define SYS_SETTING "<form method=\"GET\" action=\"saveSysConf\">"                                                                                                                                                                                                      \
                    "<label class=\"input\"><span>WiFi SSID_0(2.4G)</span><input type=\"text\"name=\"ssid_0\"value=\"%s\"></label>"                                                                                                                                     \
                    "<label class=\"input\"><span>WiFi Passwd_0</span><input type=\"text\"name=\"password_0\"value=\"%s\"></label>"                                                                                                                                     \
                    "<label class=\"input\"><span>功耗控制（0低发热 1性能优先）</span><input type=\"text\"name=\"power_mode\"value=\"%s\"></label>"                                                                                                        \
                    "<label class=\"input\"><span>屏幕亮度 (值为1~100)</span><input type=\"text\"name=\"backLight\"value=\"%s\"></label>"                                                                                                                         \
                    "<label class=\"input\"><span>屏幕方向 (0~5可选)</span><input type=\"text\"name=\"rotation\"value=\"%s\"></label>"                                                                                                                            \
                    "<label class=\"input\"><span>操作方向（0~15可选）</span><input type=\"text\"name=\"mpu_order\"value=\"%s\"></label>"                                                                                                                       \
                    "<label class=\"input\"><span>MPU6050自动校准</span><input class=\"radio\" type=\"radio\" value=\"0\" name=\"auto_calibration_mpu\" %s>关闭<input class=\"radio\" type=\"radio\" value=\"1\" name=\"auto_calibration_mpu\" %s>开启</label>" \
                    "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define RGB_SETTING "<form method=\"GET\" action=\"saveRgbConf\">"                                                                                             \
                    "<label class=\"input\"><span>RGB最低亮度（0.00~0.99可选）</span><input type=\"text\"name=\"min_brightness\"value=\"%s\"></label>" \
                    "<label class=\"input\"><span>RGB最高亮度（0.00~0.99可选）</span><input type=\"text\"name=\"max_brightness\"value=\"%s\"></label>" \
                    "<label class=\"input\"><span>RGB渐变时间（整数毫秒值）</span><input type=\"text\"name=\"time\"value=\"%s\"></label>"           \
                    "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define WEATHER_SETTING "<form method=\"GET\" action=\"saveWeatherConf\">"                                                                                          \
                        "<label class=\"input\"><span>TianQi AppId</span><input type=\"text\"name=\"tianqi_appid\"value=\"%s\"></label>"                            \
                        "<label class=\"input\"><span>TianQi AppSecret</span><input type=\"text\"name=\"tianqi_appsecret\"value=\"%s\"></label>"                    \
                        "<label class=\"input\"><span>TianQi 城市名（中文）</span><input type=\"text\"name=\"tianqi_addr\"value=\"%s\"></label>"             \
                        "<label class=\"input\"><span>天气更新周期（毫秒）</span><input type=\"text\"name=\"weatherUpdataInterval\"value=\"%s\"></label>" \
                        "<label class=\"input\"><span>日期更新周期（毫秒）</span><input type=\"text\"name=\"timeUpdataInterval\"value=\"%s\"></label>"    \
                        "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define WEATHER_OLD_SETTING "<form method=\"GET\" action=\"saveWeatherOldConf\">"                                                                                       \
                            "<label class=\"input\"><span>知心天气 城市名（拼音）</span><input type=\"text\"name=\"cityname\"value=\"%s\"></label>"          \
                            "<label class=\"input\"><span>City Language(zh-Hans)</span><input type=\"text\"name=\"language\"value=\"%s\"></label>"                      \
                            "<label class=\"input\"><span>Weather Key</span><input type=\"text\"name=\"weather_key\"value=\"%s\"></label>"                              \
                            "<label class=\"input\"><span>天气更新周期（毫秒）</span><input type=\"text\"name=\"weatherUpdataInterval\"value=\"%s\"></label>" \
                            "<label class=\"input\"><span>日期更新周期（毫秒）</span><input type=\"text\"name=\"timeUpdataInterval\"value=\"%s\"></label>"    \
                            "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define BILIBILI_SETTING "<form method=\"GET\" action=\"saveBiliConf\">"                                                                                      \
                         "<label class=\"input\"><span>Bili UID</span><input type=\"text\"name=\"bili_uid\"value=\"%s\"></label>"                             \
                         "<label class=\"input\"><span>数据更新周期（毫秒）</span><input type=\"text\"name=\"updataInterval\"value=\"%s\"></label>" \
                         "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define PICTURE_SETTING "<form method=\"GET\" action=\"savePictureConf\">"                                                                                         \
                        "<label class=\"input\"><span>自动切换时间间隔（毫秒）</span><input type=\"text\"name=\"switchInterval\"value=\"%s\"></label>" \
                        "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define MEDIA_SETTING "<form method=\"GET\" action=\"saveMediaConf\">"                                                                                             \
                      "<label class=\"input\"><span>自动切换(0不切换 1自动切换)</span><input type=\"text\"name=\"switchFlag\"value=\"%s\"></label>" \
                      "<label class=\"input\"><span>功耗控制(0低发热 1性能优先)</span><input type=\"text\"name=\"powerFlag\"value=\"%s\"></label>"  \
                      "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

#define SCREEN_SETTING "<form method=\"GET\" action=\"saveScreenConf\">"                                                                                             \
                      "<label class=\"input\"><span>功耗控制(0低发热 1性能优先)</span><input type=\"text\"name=\"powerFlag\"value=\"%s\"></label>"  \
                      "</label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"保存\"></form>"

void init_page_header()
{
    const char *temp;
    temp = "<html lang='en'><head><meta charset='UTF-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Webserver</title></head><style>    body{        margin:0;        padding:0;    }    .container{        box-sizing: border-box;        width:100vw;        font-size:16px;    }    .container .nav{        display: flex;        flex-direction: row;        flex-wrap: nowrap;        background-color: #fcfcfc;        justify-content: space-between;        box-sizing: border-box;        padding-left:100px;        padding-right:20px;        padding-top:5px;        padding-bottom: 5px;        box-shadow: 0 2px 8px #f0f1f2;    }    .container .nav li{        list-style: none;        display: block;        box-sizing: border-box;        padding:5px 8px;        background-color: #fafafa;        border-radius: 15px;        transition: all .8s;    }    .container .nav li a{        color:#333;        text-align:center;        text-decoration: none;            }    .container .nav li a:focus{        color:#1890ff;        border-top:3px solid #1890ff;    }    .footer{        background-color: rgba(0, 0, 0, .45);        position: fixed;        border-radius: 20px;        bottom: 20px;        width:80%;        left:10%;        text-align: center;        padding:4px 0;        color:#eaeaea;        border-top:1px solid rgba(0, 0, 0, .75);    }        h1{        color:white;        border-radius: 10px;        padding:0.2em;        background-color: #4c4c4d;    }    h2{        color:orange;    }    table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}    th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}    tr:nth-child(odd) {background-color:#eeeeee;}    .rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}    .rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}    .rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}    .column{float:left;width:50%;height:45%;}    .row:after{content:'';display:table;clear:both;}    .buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}    button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}    *{box-sizing:border-box;}    .input {display: block;margin-top: 10px;}    .input span {width: 300px;float: left;float: left;height: 36px;line-height: 36px;}    .input input {height: 30px;width: 200px;}    .input .radio {height: 30px;width: 50px;}    .btn {width: 120px;height: 35px;background-color: #000000;border: 0px;color: #ffffff;margin-top: 15px;margin-left: auto;}</style><body><div class='container'><div class='nav'><li><a href='/'>Home</a></li><li><a href='/download'>Download</a></li><li><a href='/upload'>Upload</a></li><li><a href='/delete'>Delete</a></li><li><a href='/sys_setting'>系统设置</a></li><li><a href='/rgb_setting'>RGB设置</a></li><li><a href='/weather_setting'>系统天气</a></li><li><a href='/picture_setting'>相册</a></li><li><a href='/media_setting'>媒体播放器</a></li><li><a href='/screen_setting'>屏幕分享</a></li></div>";
    webpage_header = F(temp);
}

void init_page_footer()
{
    const char * temp = "<div class=\"footer\"><p>版权所有 &copy;黄悠回 2022</p></div></div></body></html>";
    webpage_footer = F(temp);
}

// All supporting functions from here...
void HomePage()
{
    // 指定 target='_blank' 设置新建页面
    // webpage = F("<a href='https://' target='_blank'><button>xxx</button></a>");
    // webpage += F("<a href='https://' target='_blank'><button>xxxxx</button></a>");
    Send_HTML(webpage);
}

void sys_setting()
{
    char buf[2048];
    char ssid_0[32];
    char password_0[32];
    char power_mode[32];
    char backLight[32];
    char rotation[32];
    char mpu_order[32];
    char min_brightness[32];
    char max_brightness[32];
    char time[32];
    char auto_calibration_mpu[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"ssid_0", ssid_0);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"password_0", password_0);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"power_mode", power_mode);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"backLight", backLight);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"rotation", rotation);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"mpu_order", mpu_order);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"min_brightness", min_brightness);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"max_brightness", max_brightness);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"time", time);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"auto_calibration_mpu", auto_calibration_mpu);
    SysUtilConfig cfg = app_controller->sys_cfg;
    // 主要为了处理启停MPU自动校准的单选框
    if (0 == cfg.auto_calibration_mpu)
    {
        sprintf(buf, SYS_SETTING,
                ssid_0, password_0,
                power_mode, backLight, rotation,
                mpu_order, "checked=\"checked\"", "");
    }
    else
    {
        sprintf(buf, SYS_SETTING,
                ssid_0, password_0,
                power_mode, backLight, rotation,
                mpu_order, "", "checked=\"checked\"");
    }
    webpage = buf;
    Send_HTML(webpage);
}

void rgb_setting()
{
    char buf[2048];
    char min_brightness[32];
    char max_brightness[32];
    char time[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"min_brightness", min_brightness);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"max_brightness", max_brightness);
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_GET_PARAM,
                            (void *)"time", time);
    sprintf(buf, RGB_SETTING,
            min_brightness, max_brightness, time);
    webpage = buf;
    Send_HTML(webpage);
}

void weather_setting()
{
    char buf[2048];
    char tianqi_appid[32];
    char tianqi_appsecret[32];
    char tianqi_addr[32];
    char weatherUpdataInterval[32];
    char timeUpdataInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_GET_PARAM,
                            (void *)"tianqi_appid", tianqi_appid);
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_GET_PARAM,
                            (void *)"tianqi_appsecret", tianqi_appsecret);
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_GET_PARAM,
                            (void *)"tianqi_addr", tianqi_addr);
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_GET_PARAM,
                            (void *)"weatherUpdataInterval", weatherUpdataInterval);
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_GET_PARAM,
                            (void *)"timeUpdataInterval", timeUpdataInterval);
    sprintf(buf, WEATHER_SETTING, tianqi_appid,
            tianqi_appsecret, tianqi_addr,
            weatherUpdataInterval,
            timeUpdataInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void weather_old_setting()
{
    char buf[2048];
    char cityname[32];
    char language[32];
    char weather_key[32];
    char weatherUpdataInterval[32];
    char timeUpdataInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_GET_PARAM,
                            (void *)"cityname", cityname);
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_GET_PARAM,
                            (void *)"language", language);
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_GET_PARAM,
                            (void *)"weather_key", weather_key);
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_GET_PARAM,
                            (void *)"weatherUpdataInterval", weatherUpdataInterval);
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_GET_PARAM,
                            (void *)"timeUpdataInterval", timeUpdataInterval);
    sprintf(buf, WEATHER_OLD_SETTING,
            cityname,
            language,
            weather_key,
            weatherUpdataInterval,
            timeUpdataInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void bili_setting()
{
    char buf[2048];
    char bili_uid[32];
    char updataInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Bili", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Bili", APP_MESSAGE_GET_PARAM,
                            (void *)"bili_uid", bili_uid);
    app_controller->send_to(SERVER_APP_NAME, "Bili", APP_MESSAGE_GET_PARAM,
                            (void *)"updataInterval", updataInterval);
    sprintf(buf, BILIBILI_SETTING, bili_uid, updataInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void picture_setting()
{
    char buf[2048];
    char switchInterval[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Picture", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Picture", APP_MESSAGE_GET_PARAM,
                            (void *)"switchInterval", switchInterval);
    sprintf(buf, PICTURE_SETTING, switchInterval);
    webpage = buf;
    Send_HTML(webpage);
}

void media_setting()
{
    char buf[2048];
    char switchFlag[32];
    char powerFlag[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Media", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Media", APP_MESSAGE_GET_PARAM,
                            (void *)"switchFlag", switchFlag);
    app_controller->send_to(SERVER_APP_NAME, "Media", APP_MESSAGE_GET_PARAM,
                            (void *)"powerFlag", powerFlag);
    sprintf(buf, MEDIA_SETTING, switchFlag, powerFlag);
    webpage = buf;
    Send_HTML(webpage);
}

void screen_setting()
{
    char buf[2048];
    char powerFlag[32];
    // 读取数据
    app_controller->send_to(SERVER_APP_NAME, "Screen share", APP_MESSAGE_READ_CFG,
                            NULL, NULL);
    app_controller->send_to(SERVER_APP_NAME, "Screen share", APP_MESSAGE_GET_PARAM,
                            (void *)"powerFlag", powerFlag);
    sprintf(buf, SCREEN_SETTING, powerFlag);
    webpage = buf;
    Send_HTML(webpage);
}

void saveSysConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));

    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"ssid_0",
                            (void *)server.arg("ssid_0").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"password_0",
                            (void *)server.arg("password_0").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"power_mode",
                            (void *)server.arg("power_mode").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"backLight",
                            (void *)server.arg("backLight").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"rotation",
                            (void *)server.arg("rotation").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"mpu_order",
                            (void *)server.arg("mpu_order").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"auto_calibration_mpu",
                            (void *)server.arg("auto_calibration_mpu").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveRgbConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));

    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"min_brightness",
                            (void *)server.arg("min_brightness").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"max_brightness",
                            (void *)server.arg("max_brightness").c_str());
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"time",
                            (void *)server.arg("time").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "AppCtrl", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveWeatherConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));

    app_controller->send_to(SERVER_APP_NAME, "Weather",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"tianqi_appid",
                            (void *)server.arg("tianqi_appid").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"tianqi_appsecret",
                            (void *)server.arg("tianqi_appsecret").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"tianqi_addr",
                            (void *)server.arg("tianqi_addr").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"weatherUpdataInterval",
                            (void *)server.arg("weatherUpdataInterval").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"timeUpdataInterval",
                            (void *)server.arg("timeUpdataInterval").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Weather", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveWeatherOldConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));

    app_controller->send_to(SERVER_APP_NAME, "Weather Old",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"cityname",
                            (void *)server.arg("cityname").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather Old",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"language",
                            (void *)server.arg("language").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather Old",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"weather_key",
                            (void *)server.arg("weather_key").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather Old",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"weatherUpdataInterval",
                            (void *)server.arg("weatherUpdataInterval").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Weather Old",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"timeUpdataInterval",
                            (void *)server.arg("timeUpdataInterval").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Weather Old", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveBiliConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
    app_controller->send_to(SERVER_APP_NAME, "Bili",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"bili_uid",
                            (void *)server.arg("bili_uid").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Bili",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"updataInterval",
                            (void *)server.arg("updataInterval").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Bili", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void savePictureConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
    app_controller->send_to(SERVER_APP_NAME, "Picture",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchInterval",
                            (void *)server.arg("switchInterval").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Picture", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveMediaConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
    app_controller->send_to(SERVER_APP_NAME, "Media",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"switchFlag",
                            (void *)server.arg("switchFlag").c_str());
    app_controller->send_to(SERVER_APP_NAME, "Media",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"powerFlag",
                            (void *)server.arg("powerFlag").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Media", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void saveScreenConf(void)
{
    Send_HTML(F("<h1>设置成功! 退出APP或者继续其他设置.</h1>"));
    app_controller->send_to(SERVER_APP_NAME, "Screen share",
                            APP_MESSAGE_SET_PARAM,
                            (void *)"powerFlag",
                            (void *)server.arg("powerFlag").c_str());
    // 持久化数据
    app_controller->send_to(SERVER_APP_NAME, "Screen share", APP_MESSAGE_WRITE_CFG,
                            NULL, NULL);
}

void File_Delete()
{
    Send_HTML(
        F("<h3>Enter filename to delete</h3>"
          "<form action='/delete_result' method='post'>"
          "<input type='text' name='delete_filepath' placeHolder='绝对路径 /image/...'><br>"
          "</label><input class=\"btn\" type=\"submit\" name=\"Submie\" value=\"确认删除\"></form>"
          "<a href='/'>[Back]</a>"));
}

void delete_result(void)
{
    String del_file = server.arg("delete_filepath");
    boolean ret = tf.deleteFile(del_file);
    if (ret)
    {
        webpage = "<h3>Delete succ!</h3><a href='/delete'>[Back]</a>";
        tf.listDir("/image", 250);
    }
    else
    {
        webpage = "<h3>Delete fail! Please check up file path.</h3><a href='/delete'>[Back]</a>";
    }
    tf.listDir("/image", 250);
    Send_HTML(webpage);
}

void File_Download()
{ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
    if (server.args() > 0)
    { // Arguments were received
        if (server.hasArg("download"))
            sd_file_download(server.arg(0));
    }
    else
        SelectInput("Enter filename to download", "download", "download");
}

void sd_file_download(const String &filename)
{
    if (sd_present)
    {
        File download = tf.open("/" + filename);
        if (download)
        {
            server.sendHeader("Content-Type", "text/text");
            server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
            server.sendHeader("Connection", "close");
            server.streamFile(download, "application/octet-stream");
            download.close();
        }
        else
            ReportFileNotPresent(String("download"));
    }
    else
        ReportSDNotPresent();
}

void File_Upload()
{
    tf.listDir("/image", 250);

    webpage = webpage_header;
    webpage += "<h3>Select File to Upload</h3>"
               "<FORM action='/fupload' method='post' enctype='multipart/form-data'>"
               "<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>"
               "<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>"
               "<a href='/'>[Back]</a><br><br>";
    webpage += webpage_footer;
    server.send(200, "text/html", webpage);
}

File UploadFile;
void handleFileUpload()
{                                                   // upload a new file to the Filing system
    HTTPUpload &uploadFileStream = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                                    // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
    String filename = uploadFileStream.filename;
    if (uploadFileStream.status == UPLOAD_FILE_START)
    {
        // String filename = uploadFileStream.filename;
        // if (!filename.startsWith("/image"))
        filename = "/image/" + filename;
        Serial.print(F("Upload File Name: "));
        Serial.println(filename);
        tf.deleteFile(filename);                    // Remove a previous version, otherwise data is appended the file again
        UploadFile = tf.open(filename, FILE_WRITE); // Open the file for writing in SPIFFS (create it, if doesn't exist)
    }
    else if (uploadFileStream.status == UPLOAD_FILE_WRITE)
    {
        if (UploadFile)
            UploadFile.write(uploadFileStream.buf, uploadFileStream.currentSize); // Write the received bytes to the file
    }
    else if (uploadFileStream.status == UPLOAD_FILE_END)
    {
        if (UploadFile) // If the file was successfully created
        {
            UploadFile.close(); // Close the file again
            Serial.print(F("Upload Size: "));
            Serial.println(uploadFileStream.totalSize);
            webpage = webpage_header;
            webpage += F("<h3>File was successfully uploaded</h3>");
            webpage += F("<h2>Uploaded File Name: ");
            webpage += filename + "</h2>";
            webpage += F("<h2>File Size: ");
            webpage += file_size(uploadFileStream.totalSize) + "</h2><br>";
            webpage += webpage_footer;
            server.send(200, "text/html", webpage);
            tf.listDir("/image", 250);
        }
        else
        {
            ReportCouldNotCreateFile(String("upload"));
        }
    }
}

void SelectInput(String heading, String command, String arg_calling_name)
{
    webpage = F("<h3>");
    webpage += heading + "</h3>";
    webpage += F("<FORM action='/");
    webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
    webpage += F("<input type='text' name='");
    webpage += arg_calling_name;
    webpage += F("' value=''><br>");
    webpage += F("<type='submit' name='");
    webpage += arg_calling_name;
    webpage += F("' value=''><br>");
    webpage += F("<a href='/'>[Back]</a>");
    Send_HTML(webpage);
}

void ReportSDNotPresent()
{
    webpage = F("<h3>No SD Card present</h3>");
    webpage += F("<a href='/'>[Back]</a><br><br>");
    Send_HTML(webpage);
}

void ReportFileNotPresent(const String &target)
{
    webpage = F("<h3>File does not exist</h3>");
    webpage += F("<a href='/");
    webpage += target + "'>[Back]</a><br><br>";
    Send_HTML(webpage);
}

void ReportCouldNotCreateFile(const String &target)
{
    webpage = F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
    webpage += F("<a href='/");
    webpage += target + "'>[Back]</a><br><br>";
    Send_HTML(webpage);
}