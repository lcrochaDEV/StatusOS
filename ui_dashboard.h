#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "WifiConnect.h"
#include "FileSystemControll.h"

// ==========================================
// PALETA DE CORES (DARK MODE NEON)
// ==========================================
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

    constexpr lv_color_t TEXT_WHITE    = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);
    constexpr lv_color_t TEXT_MUTED    = LV_COLOR_MAKE(0x8F, 0x94, 0xB8);
    constexpr lv_color_t GREEN_ONLINE  = LV_COLOR_MAKE(0x00, 0xFF, 0x88);
}

// ==========================================
// CLASSE PRINCIPAL DASHBOARD UI
// ==========================================
class DashboardUI {
private:
    // Referências necessárias apenas para atualização dinâmica
    lv_obj_t *label_host = nullptr;
    lv_obj_t *label_wifi = nullptr;

    // Popups e Diálogos Ativos
    lv_obj_t *msgbox_wifi = nullptr;
    lv_obj_t *msgbox_brilho = nullptr;
    lv_obj_t *label_brilho_pct = nullptr;

    // Componentes de Métricas (Arrays para iteração limpa)
    lv_obj_t *metric_cards[5] = {nullptr};
    lv_obj_t *arcs[4]         = {nullptr};
    lv_obj_t *labels_val[4]   = {nullptr};

    lv_obj_t *lbl_os_name = nullptr;
    lv_obj_t *lbl_os_kernel = nullptr;

    // --------------------------------------------------
    // HELPERS DRY / FACTORIES INTERNAS
    // --------------------------------------------------

    // Configura o estilo padrão escuro de um Popup
    lv_obj_t* create_base_popup(const char* title_text) {
        lv_obj_t* msgbox = lv_msgbox_create(lv_screen_active());
        if (!msgbox) return nullptr;

        lv_obj_set_style_bg_color(msgbox, UITheme::CARD_BG, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(msgbox, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(msgbox, UITheme::BORDER_ACTIVE, LV_PART_MAIN);
        lv_obj_set_style_border_width(msgbox, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(msgbox, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_all(msgbox, 0, LV_PART_MAIN);
        lv_obj_set_style_clip_corner(msgbox, true, LV_PART_MAIN);

        lv_obj_t * title = lv_msgbox_add_title(msgbox, title_text);
        lv_obj_t * header = lv_msgbox_get_header(msgbox);
        if (header) {
            lv_obj_remove_style_all(header);
            lv_obj_set_style_bg_color(header, UITheme::TOPBAR, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_pad_all(header, 10, LV_PART_MAIN);
        }

        if (title) {
            lv_obj_set_width(title, lv_pct(100));
            lv_obj_set_style_text_color(title, UITheme::TEXT_WHITE, LV_PART_MAIN);
            lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        }

        return msgbox;
    }

    // Helper genérico para botões de rodapé nos popups
    void setup_footer_button(lv_obj_t* msgbox, const char* text, lv_event_cb_t cb) {
        lv_obj_t * btn = lv_msgbox_add_footer_button(msgbox, text);
        if (btn) {
            lv_obj_set_style_bg_color(btn, UITheme::BORDER_ACTIVE, LV_PART_MAIN);
            lv_obj_set_style_text_color(btn, UITheme::TEXT_WHITE, LV_PART_MAIN);
            lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, this);
        }
    }

    // Criador de Cards de Métrica
    void create_metric_card(lv_obj_t *parent, int idx, int x, int y, int w, int h, 
                            lv_color_t card_color, const char *title, lv_color_t arc_color) {
        lv_obj_t *card = lv_obj_create(parent);
        if (!card) return;

        metric_cards[idx] = card;
        lv_obj_set_size(card, w, h);
        lv_obj_set_pos(card, x, y);
        lv_obj_set_style_bg_color(card, card_color, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card, UITheme::BORDER_IDLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_title = lv_label_create(card);
        if (lbl_title) {
            lv_label_set_text(lbl_title, title);
            lv_obj_set_style_text_color(lbl_title, UITheme::TEXT_MUTED, LV_PART_MAIN);
            lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);
        }

        lv_obj_t *arc = lv_arc_create(card);
        if (arc) {
            arcs[idx] = arc;
            lv_obj_set_size(arc, 52, 52);
            lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);
            lv_arc_set_rotation(arc, 270);
            lv_arc_set_bg_angles(arc, 0, 360);
            lv_arc_set_range(arc, 0, 100);
            lv_arc_set_value(arc, 0);
            lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
            lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_arc_color(arc, UITheme::BG_MAIN, LV_PART_MAIN);
            lv_obj_set_style_arc_color(arc, arc_color, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);

            labels_val[idx] = lv_label_create(arc);
            if (labels_val[idx]) {
                lv_label_set_text(labels_val[idx], "--");
                lv_obj_set_style_text_color(labels_val[idx], UITheme::TEXT_WHITE, LV_PART_MAIN);
                lv_obj_center(labels_val[idx]);
            }
        }
    }

    // --------------------------------------------------
    // CALLBACKS DE EVENTOS DE HARDWARE / TOUCH
    // --------------------------------------------------
    static void wifi_event_cb(lv_event_t * e) {
        auto * ui = static_cast<DashboardUI*>(lv_event_get_user_data(e));
        if (ui) ui->mostrar_popup_wifi();
    }

    static void settings_event_cb(lv_event_t * e) {
        auto * ui = static_cast<DashboardUI*>(lv_event_get_user_data(e));
        if (ui) ui->mostrar_popup_brilho();
    }

    // Callback executado dinamicamente ao deslizar o dedo no Slider da tela
    static void slider_brilho_event_cb(lv_event_t * e) {
        auto * ui = static_cast<DashboardUI*>(lv_event_get_user_data(e)); //[cite: 4]
        auto * slider = static_cast<lv_obj_t*>(lv_event_get_target(e)); //[cite: 4]
        if (!slider || !ui) return;

        int val = (int)lv_slider_get_value(slider);

        // 1. Atualiza o texto na interface em tempo real
        if (ui->label_brilho_pct) {
            lv_label_set_text_fmt(ui->label_brilho_pct, "Brilho: %d%%", val); //[cite: 4]
        }

        // 2. Interage diretamente com o Hardware PWM
        g_file_system_ctrl.setBrilhoPorcentagem(val);
        Serial.printf("[UI->Hardware] Brilho alterado via Touch: %d%%\n", val);
    }

    static void fechar_popup_wifi_cb(lv_event_t * e) {
        auto * ui = static_cast<DashboardUI*>(lv_event_get_user_data(e));
        if (ui) ui->fechar_popup_wifi();
    }

    static void fechar_popup_brilho_cb(lv_event_t * e) {
        auto * ui = static_cast<DashboardUI*>(lv_event_get_user_data(e));
        if (ui) ui->fechar_popup_brilho();
    }

public:
    DashboardUI() = default;

    // --------------------------------------------------
    // MÉTODOS PÚBLICOS DE INTERFACE
    // --------------------------------------------------

    void mostrar_popup_wifi() {
        if (msgbox_wifi != nullptr) return;

        msgbox_wifi = create_base_popup("Informacões de Rede");
        if (!msgbox_wifi) return;

        char info_buffer[96];
        snprintf(info_buffer, sizeof(info_buffer), "IP: %s\nMAC: %s", WifiConnect::obtainIP(), WifiConnect::obtainMacAddress());

        lv_obj_t * content = lv_msgbox_get_content(msgbox_wifi);
        if (content) {
            lv_obj_set_style_bg_color(content, UITheme::CARD_BG, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_pad_all(content, 16, LV_PART_MAIN);
        }

        lv_obj_t * text = lv_msgbox_add_text(msgbox_wifi, info_buffer);
        if (text) {
            lv_obj_set_style_text_color(text, UITheme::TEXT_WHITE, LV_PART_MAIN);
            lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        }

        setup_footer_button(msgbox_wifi, "Fechar", fechar_popup_wifi_cb);
        lv_obj_center(msgbox_wifi);
    }

    void fechar_popup_wifi() {
        if (msgbox_wifi != nullptr) {
            lv_obj_delete(msgbox_wifi);
            msgbox_wifi = nullptr;
        }
    }

    void mostrar_popup_brilho() {
        if (msgbox_brilho != nullptr) return; //[cite: 4]

        msgbox_brilho = create_base_popup("Brilho do Display"); //[cite: 4]
        if (!msgbox_brilho) return;

        lv_obj_t * content = lv_msgbox_get_content(msgbox_brilho); //[cite: 4]
        if (content) {
            lv_obj_set_style_bg_color(content, UITheme::CARD_BG, LV_PART_MAIN); //[cite: 4]
            lv_obj_set_style_bg_opa(content, LV_OPA_COVER, LV_PART_MAIN); //[cite: 4]
            lv_obj_set_style_pad_all(content, 16, LV_PART_MAIN); //[cite: 4]
            lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN); //[cite: 4]
            lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); //[cite: 4]
        }

        // Obtém o valor atual real vindo do hardware
        uint8_t valor_atual = g_file_system_ctrl.obterBrilhoPorcentagem();

        // 1. Slider de Brilho
        lv_obj_t * slider = lv_slider_create(content); //[cite: 4]
        lv_obj_set_size(slider, 180, 12); //[cite: 4]
        lv_slider_set_range(slider, 10, 100); //[cite: 4]
        lv_slider_set_value(slider, valor_atual, LV_ANIM_OFF); //[cite: 4]
        
        lv_obj_set_style_bg_color(slider, UITheme::ARC_CPU, LV_PART_INDICATOR); //[cite: 4]
        lv_obj_set_style_bg_color(slider, UITheme::BORDER_IDLE, LV_PART_MAIN); //[cite: 4]
        lv_obj_add_event_cb(slider, slider_brilho_event_cb, LV_EVENT_VALUE_CHANGED, this); //[cite: 4]

        // 2. Texto com a porcentagem
        label_brilho_pct = lv_label_create(content); //[cite: 4]
        if (label_brilho_pct) {
            lv_label_set_text_fmt(label_brilho_pct, "Brilho: %d%%", valor_atual); //[cite: 4]
            lv_obj_set_style_text_color(label_brilho_pct, UITheme::TEXT_WHITE, LV_PART_MAIN); //[cite: 4]
            lv_obj_set_style_pad_top(label_brilho_pct, 10, LV_PART_MAIN); //[cite: 4]
        }

        setup_footer_button(msgbox_brilho, "OK", fechar_popup_brilho_cb); //[cite: 4]
        lv_obj_center(msgbox_brilho); //[cite: 4]
    }

    void fechar_popup_brilho() {
        if (msgbox_brilho != nullptr) {
            lv_obj_delete(msgbox_brilho);
            msgbox_brilho = nullptr;
            label_brilho_pct = nullptr;
        }
    }

    void init() {
        lv_obj_t *scr = lv_screen_active();
        if (!scr) return;

        lv_obj_set_style_bg_color(scr, UITheme::BG_MAIN, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

        // --- TOPBAR ---
        lv_obj_t *topbar = lv_obj_create(scr);
        if (topbar) {
            lv_obj_set_size(topbar, 320, 32);
            lv_obj_set_pos(topbar, 0, 0);
            lv_obj_set_style_bg_color(topbar, UITheme::TOPBAR, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(topbar, UITheme::BORDER_IDLE, LV_PART_MAIN);
            lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_border_width(topbar, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(topbar, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(topbar, 2, LV_PART_MAIN);
            lv_obj_remove_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *lbl_title = lv_label_create(topbar);
            if (lbl_title) {
                lv_label_set_text(lbl_title, "DashBoard");
                lv_obj_set_style_text_color(lbl_title, UITheme::TEXT_WHITE, LV_PART_MAIN);
                lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);
            }

            label_host = lv_label_create(topbar);
            if (label_host) {
                lv_label_set_text(label_host, "Aguardando...");
                lv_obj_set_style_text_color(label_host, UITheme::TEXT_MUTED, LV_PART_MAIN);
                lv_obj_align(label_host, LV_ALIGN_CENTER, 0, 0);
            }

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
                    lv_obj_set_style_text_color(lbl, UITheme::TEXT_WHITE, LV_PART_MAIN);
                    lv_obj_center(lbl);
                }
                lv_obj_add_event_cb(btn_settings, settings_event_cb, LV_EVENT_CLICKED, this);
            }

            // Wi-Fi
            lv_obj_t* btn_wifi = lv_button_create(topbar);
            if (btn_wifi) {
                lv_obj_set_size(btn_wifi, 26, 26);
                lv_obj_align(btn_wifi, LV_ALIGN_RIGHT_MID, -30, 0);
                lv_obj_set_style_bg_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
                lv_obj_set_style_shadow_opa(btn_wifi, LV_OPA_TRANSP, LV_PART_MAIN);

                label_wifi = lv_label_create(btn_wifi);
                if (label_wifi) {
                    lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);
                    lv_obj_set_style_text_color(label_wifi, UITheme::TEXT_MUTED, LV_PART_MAIN);
                    lv_obj_center(label_wifi);
                }
                lv_obj_add_event_cb(btn_wifi, wifi_event_cb, LV_EVENT_CLICKED, this);
            }
        }

        // Construção dos 4 Cards via Array/Loop
        struct CardConfig { const char* title; lv_color_t bg; lv_color_t arc; };
        CardConfig configs[4] = {
            {"CPU",   LV_COLOR_MAKE(0x13, 0x17, 0x2E), UITheme::ARC_CPU},
            {"RAM",   LV_COLOR_MAKE(0x12, 0x24, 0x28), UITheme::ARC_RAM},
            {"Disco", LV_COLOR_MAKE(0x23, 0x14, 0x2B), UITheme::ARC_DISK},
            {"Temp",  LV_COLOR_MAKE(0x2B, 0x12, 0x1C), UITheme::ARC_TEMP}
        };

        for (int i = 0; i < 4; i++) {
            create_metric_card(scr, i, 6 + i * 78, 38, 72, 120, configs[i].bg, configs[i].title, configs[i].arc);
        }

        // Card OS
        metric_cards[4] = lv_obj_create(scr);
        if (metric_cards[4]) {
            lv_obj_set_size(metric_cards[4], 308, 68);
            lv_obj_set_pos(metric_cards[4], 6, 164);
            lv_obj_set_style_bg_color(metric_cards[4], UITheme::TOPBAR, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(metric_cards[4], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(metric_cards[4], UITheme::BORDER_IDLE, LV_PART_MAIN);
            lv_obj_set_style_border_width(metric_cards[4], 1, LV_PART_MAIN);
            lv_obj_set_style_radius(metric_cards[4], 8, LV_PART_MAIN);
            lv_obj_set_style_pad_all(metric_cards[4], 6, LV_PART_MAIN);
            lv_obj_remove_flag(metric_cards[4], LV_OBJ_FLAG_SCROLLABLE);

            lbl_os_name = lv_label_create(metric_cards[4]);
            if (lbl_os_name) {
                lv_label_set_text(lbl_os_name, "SO: Sem Telemetria");
                lv_obj_set_style_text_color(lbl_os_name, UITheme::ARC_CPU, LV_PART_MAIN);
                lv_obj_align(lbl_os_name, LV_ALIGN_TOP_LEFT, 4, 2);
            }

            lbl_os_kernel = lv_label_create(metric_cards[4]);
            if (lbl_os_kernel) {
                lv_label_set_text(lbl_os_kernel, "Kernel: N/A | Up: --");
                lv_obj_set_style_text_color(lbl_os_kernel, UITheme::TEXT_MUTED, LV_PART_MAIN);
                lv_obj_align(lbl_os_kernel, LV_ALIGN_TOP_LEFT, 4, 24);
            }
        }

        set_online_status(strcmp(WifiConnect::obtainIP(), "0.0.0.0") != 0);
    }

    void update_display(const char* host, const char* os_name, const char* kernel, 
                        float temp, float cpu_load, int ram_pct, int disk_pct, 
                        const char* uptime) {
        if (label_host && host) lv_label_set_text(label_host, host);
        if (lbl_os_name) lv_label_set_text_fmt(lbl_os_name, "SO: %s", os_name ? os_name : "N/A");
        if (lbl_os_kernel) lv_label_set_text_fmt(lbl_os_kernel, "Kernel: %s | Up: %s", 
                                                    kernel ? kernel : "N/A", 
                                                    uptime ? uptime : "--");

        int pcts[3] = {
            (int)(cpu_load * 25.0f + 0.5f), // CPU% ajustado (considerando 4 cores)
            ram_pct,
            disk_pct
        };

        for (int i = 0; i < 3; i++) {
            int val = pcts[i] > 100 ? 100 : (pcts[i] < 0 ? 0 : pcts[i]);
            if (arcs[i]) lv_arc_set_value(arcs[i], val);
            if (labels_val[i]) lv_label_set_text_fmt(labels_val[i], "%d%%", val);
        }

        int temp_val = (int)(temp + 0.5f);
        if (arcs[3]) lv_arc_set_value(arcs[3], temp_val > 100 ? 100 : (temp_val < 0 ? 0 : temp_val));
        if (labels_val[3]) lv_label_set_text_fmt(labels_val[3], "%d°C", temp_val);
    }

    void set_online_status(bool online) {
        lv_color_t border_color = online ? UITheme::BORDER_ACTIVE : UITheme::BORDER_IDLE;
        lv_opa_t card_opa       = online ? LV_OPA_80 : LV_OPA_40;

        for (lv_obj_t* card : metric_cards) {
            if (card) {
                lv_obj_set_style_bg_opa(card, card_opa, LV_PART_MAIN);
                lv_obj_set_style_border_color(card, border_color, LV_PART_MAIN);
            }
        }

        if (label_wifi) {
            lv_label_set_text(label_wifi, online ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
            lv_obj_set_style_text_color(label_wifi, online ? UITheme::GREEN_ONLINE : UITheme::ARC_TEMP, LV_PART_MAIN);
        }
    }
};

// ==========================================
// INTERFACE DE COMPATIBILIDADE GLOBAL
// ==========================================
inline DashboardUI g_dashboard;

inline void create_dashboard_ui() { g_dashboard.init(); }
inline void ui_abrir_popup_wifi()  { g_dashboard.mostrar_popup_wifi(); }
inline void ui_fechar_popup_wifi() { g_dashboard.fechar_popup_wifi(); }
inline void ui_abrir_popup_brilho() { g_dashboard.mostrar_popup_brilho(); }
inline void ui_fechar_popup_brilho(){ g_dashboard.fechar_popup_brilho(); }

inline void update_telemetry_data(const char* host, const char* os_name, const char* kernel, 
                                  float temp, float cpu_load, int ram_pct, int disk_pct, 
                                  const char* uptime, const void* logo_url) {
    (void)logo_url;
    g_dashboard.set_online_status(true);
    g_dashboard.update_display(host, os_name, kernel, temp, cpu_load, ram_pct, disk_pct, uptime);
}

inline void update_status(bool online) {
    g_dashboard.set_online_status(online);
}

#endif // UI_DASHBOARD_H