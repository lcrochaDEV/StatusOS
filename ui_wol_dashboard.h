#ifndef UI_WOL_DASHBOARD_H
#define UI_WOL_DASHBOARD_H

#include "WifiConnect.h"
#include "SDData.h"

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

// Função auxiliar para criar um card individual de WoL
static void wol_card_click_cb(lv_event_t * e) {
    const char* mac = (const char*)lv_event_get_user_data(e);
    if (mac) {
        extern WifiConnect wifiConnect;
        wifiConnect.wakeOnLan(mac);
        Serial.printf("[WoL] Disparando Magic Packet para: %s\n", mac);
    }
}

inline lv_obj_t* create_wol_card(lv_obj_t *parent, const char* name, const char* mac, int status_type, int value_or_progress) {
    // Card Container
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 95, 95);
    lv_obj_set_style_bg_color(card, WOL_COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, WOL_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Torna clicável
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    static char mac_copy[18];
    strncpy(mac_copy, mac, sizeof(mac_copy));
    lv_obj_add_event_cb(card, wol_card_click_cb, LV_EVENT_CLICKED, (void*)mac_copy);

    // Arco indicador de status
    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, 45, 45);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 2);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, WOL_COLOR_BORDER, LV_PART_MAIN);

    if (status_type == 1) { // Online
        lv_arc_set_value(arc, 100);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_GREEN, LV_PART_INDICATOR);
    } else if (status_type == 2) { // Waking
        lv_arc_set_value(arc, value_or_progress);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_YELLOW, LV_PART_INDICATOR);
    } else { // Offline
        lv_arc_set_value(arc, 100);
        lv_obj_set_style_arc_color(arc, WOL_COLOR_RED, LV_PART_INDICATOR);
    }

    // Label de Status
    lv_obj_t *lbl_status = lv_label_create(card);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, LV_PART_MAIN);
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

    // Label do Nome
    lv_obj_t *lbl_name = lv_label_create(card);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_name, LV_ALIGN_BOTTOM_MID, 0, -2);

    return card;
}

// Classe para gerenciar os estados da tela WOL
class WolDashboardUI {
private:
    lv_obj_t *lbl_wifi = nullptr;
    lv_obj_t *btn_sd = nullptr;

public:
    void init(lv_obj_t *screen_scr) {
        // --- TOPBAR PADRONIZADA ---
        lv_obj_t *topbar = lv_obj_create(screen_scr);
        lv_obj_set_size(topbar, 320, 32);
        lv_obj_set_pos(topbar, 0, 0);
        lv_obj_set_style_bg_color(topbar, lv_color_hex(0x12131C), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(topbar, lv_color_hex(0x222436), LV_PART_MAIN);
        lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
        lv_obj_set_style_border_width(topbar, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(topbar, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(topbar, 2, LV_PART_MAIN);
        lv_obj_remove_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_title = lv_label_create(topbar);
        lv_label_set_text(lbl_title, "WakeONLan");
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);

        // Engrenagem
        lv_obj_t* btn_settings = lv_button_create(topbar);
        if (btn_settings) {
            lv_obj_set_size(btn_settings, 26, 26);
            lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -2, 0);
            lv_obj_set_style_bg_opa(btn_settings, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_shadow_opa(btn_settings, LV_OPA_TRANSP, LV_PART_MAIN);

            lv_obj_t* lbl = lv_label_create(btn_settings);
            if (lbl) {
                lv_label_set_text(lbl, LV_SYMBOL_SETTINGS);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                lv_obj_center(lbl);
            }
        }

        // Wi-Fi
        lv_obj_t* btn_wifi = lv_button_create(topbar);
        if (btn_wifi) {
            lv_obj_set_size(btn_wifi, 26, 26);
            lv_obj_align(btn_wifi, LV_ALIGN_RIGHT_MID, -30, 0);
            lv_obj_set_style_bg_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_shadow_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);

            lbl_wifi = lv_label_create(btn_wifi);
            if (lbl_wifi) {
                lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
                lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(0x8F94B8), LV_PART_MAIN);
                lv_obj_center(lbl_wifi);
            }
        }

        // SD Card
        btn_sd = lv_button_create(topbar);
        if (btn_sd) {
            lv_obj_set_size(btn_sd, 26, 26);
            lv_obj_align(btn_sd, LV_ALIGN_RIGHT_MID, -58, 0);
            lv_obj_set_style_bg_opa(btn_sd, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_shadow_opa(btn_sd, LV_OPA_TRANSP, LV_PART_MAIN);

            lv_obj_t* lbl_sd = lv_label_create(btn_sd);
            if (lbl_sd) {
                lv_label_set_text(lbl_sd, LV_SYMBOL_SD_CARD);
                lv_obj_set_style_text_color(lbl_sd, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                lv_obj_center(lbl_sd);
            }
            lv_obj_add_flag(btn_sd, LV_OBJ_FLAG_HIDDEN); // Escondido por padrão
        }

        // Container Principal em Grid
        lv_obj_t *grid_container = lv_obj_create(screen_scr);
        lv_obj_set_size(grid_container, 320, 208);
        lv_obj_set_pos(grid_container, 0, 32);
        lv_obj_set_style_bg_color(grid_container, WOL_COLOR_BG, LV_PART_MAIN);
        lv_obj_set_style_border_width(grid_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(grid_container, 6, LV_PART_MAIN);

        lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        // Carregamento Dinâmico
        JsonDocument doc;
        File file = SD.open("/data/config.json", "r");
        if (file) {
            DeserializationError error = deserializeJson(doc, file);
            file.close();

            if (!error && doc.containsKey("devices")) {
                JsonArray devices = doc["devices"];
                for (JsonObject device : devices) {
                    const char* name = device["name"] | "Sem Nome";
                    const char* mac  = device["mac"]  | "00:00:00:00:00:00";
                    create_wol_card(grid_container, name, mac, 0, 100);
                }
            }
        }
    }

    void set_online_status(bool online) {
        if (lbl_wifi) {
            lv_label_set_text(lbl_wifi, online ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
            lv_obj_set_style_text_color(lbl_wifi, online ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xFF0055), LV_PART_MAIN);
        }
    }

    void check_sd_status() {
        if (!btn_sd) return;
        if (sdData.isReady()) lv_obj_add_flag(btn_sd, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(btn_sd, LV_OBJ_FLAG_HIDDEN);
    }
};

inline WolDashboardUI g_wol_dashboard;

inline void create_wol_screen(lv_obj_t *screen_scr) { g_wol_dashboard.init(screen_scr); }
inline void update_wol_status(bool online) { g_wol_dashboard.set_online_status(online); }
inline void check_wol_sd_status() { g_wol_dashboard.check_sd_status(); }

#endif // UI_WOL_DASHBOARD_H
