#ifndef UI_WOL_DASHBOARD_H
#define UI_WOL_DASHBOARD_H

#include <lvgl.h>

// Cores base do tema Neon
#define WOL_COLOR_BG       lv_color_hex(0x0A0B10)
#define WOL_COLOR_CARD     lv_color_hex(0x180C30)
#define WOL_COLOR_BORDER   lv_color_hex(0x28144D)
#define WOL_COLOR_GREEN    lv_color_hex(0x00FF88)
#define WOL_COLOR_RED      lv_color_hex(0xFF0055)
#define WOL_COLOR_YELLOW   lv_color_hex(0xFFCC00)
#define WOL_COLOR_MUTED    lv_color_hex(0x8B7CA8)

// Estrutura para gerenciar os cards de dispositivos
struct WolDeviceCard {
    lv_obj_t *card;
    lv_obj_t *arc;
    lv_obj_t *lbl_status;
    lv_obj_t *lbl_name;
};

// Exemplo de array para múltiplos dispositivos (Grid 2x3 ou ajustável)
inline WolDeviceCard wol_cards[6];

// Função auxiliar para criar um card individual de WoL
inline lv_obj_t* create_wol_card(lv_obj_t *parent, const char* name, int status_type, int value_or_progress) {
    // Card Container
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 95, 95); // Ajustado para caber em grid no display 320x240
    lv_obj_set_style_bg_color(card, WOL_COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, WOL_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Arco indicador de status (semelhante aos gauges de CPU/RAM)
    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, 45, 45);
    lv_obj_align(arc, LV_ALIGN_TOP, 0, 2);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, WOL_COLOR_BORDER, LV_PART_MAIN);

    // Configuração de Cores por Status (0: Offline, 1: Online, 2: Waking)
    if (status_type == 1) { // Online
        lv_arc_set_value(arc, 100);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_GREEN, LV_PART_INDICATOR);
    } else if (status_type == 2) { // Waking (Com progresso)
        lv_arc_set_value(arc, value_or_progress);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_YELLOW, LV_PART_INDICATOR);
    } else { // Offline
        lv_arc_set_value(arc, 100);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_RED, LV_PART_INDICATOR);
    }

    // Label de Status (Online / Offline / Waking 41s)
    lv_obj_t *lbl_status = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_10, LV_PART_MAIN);
    if (status_type == 1) {
        lv_label_set_text(lbl_status, "Online");
        lv_obj_set_style_text_color(lbl_status, WOL_COLOR_GREEN, LV_PART_MAIN);
    } else if (status_type == 2) {
        lv_label_set_text_fmt(lbl_status, "Waking %ds", value_or_progress);
        lv_obj_set_style_text_color(lbl_status, WOL_COLOR_YELLOW, LV_PART_MAIN);
    } else {
        lv_label_set_text(lbl_status, "Offline");
        lv_obj_set_style_text_color(lbl_status, WOL_COLOR_RED, LV_PART_MAIN);
    }
    lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 14);

    // Label do Nome do Dispositivo (ex: OFFICE-PC)
    lv_obj_t *lbl_name = lv_label_create(card);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_name, LV_ALIGN_BOTTOM, 0, -2);

    return card;
}

// Função para criar a Tela Secundária Completa
inline void create_wol_screen(lv_obj_t *screen_scr) {
    // Container Principal em Grid (2 linhas x 3 colunas para display de 320x240)
    lv_obj_t *grid_container = lv_obj_create(screen_scr);
    lv_obj_set_size(grid_container, 320, 208); // Abaixo da topbar de 32px
    lv_obj_set_pos(grid_container, 0, 32);
    lv_obj_set_style_bg_color(grid_container, WOL_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid_container, 6, LV_PART_MAIN);
    lv_obj_remove_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);

    // Layout Flexível ou Posicionamento Fixo em Grade (3 colunas, 2 linhas)
    // Dispositivo 1: Online
    lv_obj_align(create_wol_card(grid_container, "OFFICE-PC", 1, 100), LV_ALIGN_TOP_LEFT, 2, 2);
    // Dispositivo 2: Online
    lv_obj_align(create_wol_card(grid_container, "NAS", 1, 100), LV_ALIGN_TOP_MID, 0, 2);
    // Dispositivo 3: Offline
    lv_obj_align(create_wol_card(grid_container, "MEDIA-PC", 0, 100), LV_ALIGN_TOP_RIGHT, -2, 2);

    // Dispositivo 4: Waking (com contagem regressiva)
    lv_obj_align(create_wol_card(grid_container, "WORKSTATION", 2, 41), LV_ALIGN_BOTTOM_LEFT, 2, -2);
    // Dispositivo 5: Online
    lv_obj_align(create_wol_card(grid_container, "SERVER", 1, 100), LV_ALIGN_BOTTOM_MID, 0, -2);
    // Dispositivo 6: Offline
    lv_obj_align(create_wol_card(grid_container, "GAMING-PC", 0, 100), LV_ALIGN_BOTTOM_RIGHT, -2, -2);
}

#endif // UI_WOL_DASHBOARD_H
