#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

namespace UITheme {
    constexpr lv_color_t BG_MAIN       = LV_COLOR_MAKE(0x0A, 0x0B, 0x10);
    constexpr lv_color_t TOPBAR        = LV_COLOR_MAKE(0x12, 0x13, 0x1C);
    constexpr lv_color_t CARD_BG       = LV_COLOR_MAKE(0x2A, 0x2C, 0x3D);
    constexpr lv_color_t BORDER_IDLE   = LV_COLOR_MAKE(0x22, 0x24, 0x36);
    constexpr lv_color_t BORDER_ACTIVE = LV_COLOR_MAKE(0x3D, 0x42, 0x6B);

    constexpr lv_color_t ARC_CPU       = LV_COLOR_MAKE(0x00, 0xF2, 0xFE);
    constexpr lv_color_t ARC_RAM       = LV_COLOR_MAKE(0xFF, 0x00, 0x7F);
    constexpr lv_color_t ARC_DISK      = LV_COLOR_MAKE(0xBD, 0x00, 0xFF);
    constexpr lv_color_t ARC_TEMP      = LV_COLOR_MAKE(0xFF, 0x00, 0x55);

    // Card background colors
    constexpr lv_color_t CARD_CPU_BG   = LV_COLOR_MAKE(0x13, 0x17, 0x2E);
    constexpr lv_color_t CARD_RAM_BG   = LV_COLOR_MAKE(0x12, 0x24, 0x28);
    constexpr lv_color_t CARD_DISK_BG  = LV_COLOR_MAKE(0x23, 0x14, 0x2B);
    constexpr lv_color_t CARD_TEMP_BG  = LV_COLOR_MAKE(0x2B, 0x12, 0x1C);

    constexpr lv_color_t TEXT_WHITE    = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);
    constexpr lv_color_t TEXT_MUTED    = LV_COLOR_MAKE(0x8F, 0x94, 0xB8);

    inline void apply_header_style(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, TOPBAR, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(obj, BORDER_IDLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    }
}

namespace CryptoTheme {
    constexpr lv_color_t BG_MAIN       = UITheme::BG_MAIN;
    constexpr lv_color_t TOPBAR        = UITheme::TOPBAR;
    constexpr lv_color_t CARD_BG       = UITheme::CARD_BG;
    constexpr lv_color_t BORDER_IDLE   = UITheme::BORDER_IDLE;
    constexpr lv_color_t BORDER_ACTIVE = UITheme::BORDER_ACTIVE;

    constexpr lv_color_t UP         = LV_COLOR_MAKE(0x22, 0xC5, 0x5E);
    constexpr lv_color_t DOWN       = LV_COLOR_MAKE(0xEF, 0x44, 0x44);
    constexpr lv_color_t LOCK       = LV_COLOR_MAKE(0xFF, 0x9C, 0x9C);
    constexpr lv_color_t WARN       = LV_COLOR_MAKE(0xF3, 0xBA, 0x2F);
    constexpr lv_color_t PRICE      = LV_COLOR_MAKE(0xB6, 0xF5, 0x1B);
    constexpr lv_color_t CANDLE_UP  = LV_COLOR_MAKE(0xB6, 0xF5, 0x1B);
    constexpr lv_color_t CANDLE_DN  = LV_COLOR_MAKE(0xFF, 0x6B, 0x2C);
}

#endif
