#include "ui_header.h"

UIHeader::UIHeader(lv_obj_t* parent, const char* title, const char* text, lv_event_cb_t wifi_cb, lv_event_cb_t settings_cb, void* user_data, bool show_battery, bool show_clock) {
    container = lv_obj_create(parent);
    UITheme::apply_header_style(container);
    lv_obj_set_size(container, 320, 32);
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_style_border_side(container, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_style_text_color(titleLabel, UITheme::TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(titleLabel, LV_ALIGN_LEFT_MID, 6, 0);

    textLabel = lv_label_create(container);
    lv_label_set_text(textLabel, text);
    lv_obj_set_style_text_color(textLabel, UITheme::TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(textLabel, LV_ALIGN_CENTER, 0, 0);

    // Settings
    lv_obj_t* btn_settings = lv_button_create(container);
    lv_obj_set_size(btn_settings, 26, 26);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_opa(btn_settings, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_settings, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_t* lbl_s = lv_label_create(btn_settings);
    lv_label_set_text(lbl_s, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(lbl_s, UITheme::TEXT_WHITE, LV_PART_MAIN);
    lv_obj_center(lbl_s);
    if (settings_cb) lv_obj_add_event_cb(btn_settings, settings_cb, LV_EVENT_CLICKED, user_data);

    // Wi-Fi
    lv_obj_t* btn_wifi = lv_button_create(container);
    lv_obj_set_size(btn_wifi, 26, 26);
    lv_obj_align(btn_wifi, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_set_style_bg_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
    wifiLabel = lv_label_create(btn_wifi);
    lv_label_set_text(wifiLabel, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifiLabel, UITheme::TEXT_MUTED, LV_PART_MAIN);
    lv_obj_center(wifiLabel);
    if (wifi_cb) lv_obj_add_event_cb(btn_wifi, wifi_cb, LV_EVENT_CLICKED, user_data);

    // SD
    sdContainer = lv_button_create(container);
    lv_obj_set_size(sdContainer, 26, 26);
    lv_obj_align(sdContainer, LV_ALIGN_RIGHT_MID, -58, 0);
    lv_obj_set_style_bg_opa(sdContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(sdContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    sdLabel = lv_label_create(sdContainer);
    lv_label_set_text(sdLabel, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(sdLabel, UITheme::TEXT_WHITE, LV_PART_MAIN);
    lv_obj_center(sdLabel);
    lv_obj_add_flag(sdContainer, LV_OBJ_FLAG_HIDDEN);

    int right_offset = 84;
    if (show_battery) {
        batteryLabel = lv_label_create(container);
        lv_label_set_text(batteryLabel, LV_SYMBOL_BATTERY_FULL);
        lv_obj_set_style_text_color(batteryLabel, UITheme::TEXT_WHITE, LV_PART_MAIN);
        lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -right_offset, 0);
        right_offset += 24;
    } else {
        batteryLabel = nullptr;
    }

    if (show_clock) {
        clockLabel = lv_label_create(container);
        lv_label_set_text(clockLabel, "--:--");
        lv_obj_set_style_text_color(clockLabel, UITheme::TEXT_MUTED, LV_PART_MAIN);
        lv_obj_align(clockLabel, LV_ALIGN_RIGHT_MID, -right_offset, 0);
    } else {
        clockLabel = nullptr;
    }
}

void UIHeader::setTitle(const char* title) { lv_label_set_text(titleLabel, title); }
void UIHeader::setText(const char* text) { lv_label_set_text(textLabel, text); }
void UIHeader::updateSD(bool exists) { exists ? lv_obj_add_flag(sdContainer, LV_OBJ_FLAG_HIDDEN) : lv_obj_clear_flag(sdContainer, LV_OBJ_FLAG_HIDDEN); }
void UIHeader::updateWifi(bool connected) {
    lv_label_set_text(wifiLabel, connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(wifiLabel, connected ? UITheme::TEXT_WHITE : UITheme::ARC_TEMP, LV_PART_MAIN);
}
void UIHeader::setWifiColor(lv_color_t color) {
    if (!wifiLabel) return;
    lv_obj_set_style_text_color(wifiLabel, color, LV_PART_MAIN);
}
void UIHeader::setBattery(int percent) {
    if (!batteryLabel) return;
    const char* sym = percent >= 88 ? LV_SYMBOL_BATTERY_FULL
                    : percent >= 63 ? LV_SYMBOL_BATTERY_3
                    : percent >= 38 ? LV_SYMBOL_BATTERY_2
                    : percent >= 13 ? LV_SYMBOL_BATTERY_1
                                    : LV_SYMBOL_BATTERY_EMPTY;
    lv_label_set_text(batteryLabel, sym);
}
void UIHeader::setClock(const char* time) {
    if (!clockLabel) return;
    lv_label_set_text(clockLabel, time);
}
