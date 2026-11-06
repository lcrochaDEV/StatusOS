#ifndef UI_WOL_DASHBOARD_H
#define UI_WOL_DASHBOARD_H

#include "WifiConnect.h"
#include "SDData.h"
#include "ui_header.h"

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
    UIHeader *header = nullptr;

public:
    void init(lv_obj_t *screen_scr) {
        // --- TOPBAR PADRONIZADA ---
        header = new UIHeader(screen_scr, "WakeONLan", "", nullptr, nullptr, this);


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
        if (header) header->updateWifi(online);
    }

    void check_sd_status() {
        if (header) header->updateSD(sdData.isReady());
    }
};

inline WolDashboardUI g_wol_dashboard;

inline void create_wol_screen(lv_obj_t *screen_scr) { g_wol_dashboard.init(screen_scr); }
inline void update_wol_status(bool online) { g_wol_dashboard.set_online_status(online); }
inline void check_wol_sd_status() { g_wol_dashboard.check_sd_status(); }

#endif // UI_WOL_DASHBOARD_H
