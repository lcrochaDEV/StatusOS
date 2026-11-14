#ifndef UI_HEADER_H
#define UI_HEADER_H

#include <lvgl.h>
#include "UITheme.h"

class UIHeader {
private:
    lv_obj_t* container;
    lv_obj_t* titleLabel;
    lv_obj_t* textLabel;
    lv_obj_t* wifiLabel;
    lv_obj_t* sdContainer;
    lv_obj_t* sdLabel;
    lv_obj_t* batteryLabel;
    lv_obj_t* clockLabel;

public:
    UIHeader(lv_obj_t* parent, const char* title, const char* text, lv_event_cb_t wifi_cb, lv_event_cb_t settings_cb, void* user_data, bool show_battery = false, bool show_clock = false);
    void setTitle(const char* title);
    void setText(const char* text);
    void updateSD(bool exists);
    void updateWifi(bool connected);
    void setWifiColor(lv_color_t color);
    void setBattery(int percent);
    void setClock(const char* time);
};

#endif
