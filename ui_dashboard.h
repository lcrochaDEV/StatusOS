#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// ==========================================
// PALETA DE CORES - DARK MODE NEON
// ==========================================
#define COLOR_BG_MAIN       lv_color_hex(0x0A0B10)
#define COLOR_TOPBAR        lv_color_hex(0x12131C)
#define COLOR_BORDER_IDLE   lv_color_hex(0x222436)
#define COLOR_BORDER_ACTIVE lv_color_hex(0x3D426B)

// Cartões Escuros Neon (Dark Cards)
#define COLOR_CARD_CPU      lv_color_hex(0x13172E) 
#define COLOR_ARC_CPU       lv_color_hex(0x00F2FE) 

#define COLOR_CARD_RAM      lv_color_hex(0x122428) 
#define COLOR_ARC_RAM       lv_color_hex(0xFF007F) 

#define COLOR_CARD_DISK     lv_color_hex(0x23142B) 
#define COLOR_ARC_DISK      lv_color_hex(0xBD00FF) 

#define COLOR_CARD_TEMP     lv_color_hex(0x2B121C) 
#define COLOR_ARC_TEMP      lv_color_hex(0xFF0055) 

#define COLOR_TEXT_WHITE    lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_MUTED    lv_color_hex(0x8F94B8)
#define COLOR_GREEN_ONLINE  lv_color_hex(0x00FF88)

// ==========================================
// CLASSE COMPONENTIZADA DASHBOARD UI
// ==========================================
class DashboardUI {
private:
    // Ponteiros Globais Internos
    lv_obj_t *label_host = nullptr;
    lv_obj_t *label_status = nullptr;

    lv_obj_t *card_cpu = nullptr;
    lv_obj_t *card_ram = nullptr;
    lv_obj_t *card_disk = nullptr;
    lv_obj_t *card_temp = nullptr;
    lv_obj_t *card_os = nullptr;

    lv_obj_t *arc_cpu = nullptr;
    lv_obj_t *label_cpu_val = nullptr;

    lv_obj_t *arc_ram = nullptr;
    lv_obj_t *label_ram_val = nullptr;

    lv_obj_t *arc_disk = nullptr;
    lv_obj_t *label_disk_val = nullptr;

    lv_obj_t *arc_temp = nullptr;
    lv_obj_t *lbl_temp = nullptr;

    lv_obj_t *lbl_os_name = nullptr;
    lv_obj_t *lbl_os_kernel = nullptr;

    // Helper DRY reutilizável para criação de cada card com fundo forte (LV_OPA_COVER)
    lv_obj_t* create_metric_card(lv_obj_t *parent, int x, int y, int width, int height, 
                                 lv_color_t card_color, const char *title, 
                                 lv_color_t arc_color, lv_obj_t **out_arc, lv_obj_t **out_label_val) {
        lv_obj_t *card = lv_obj_create(parent);
        if (!card) return nullptr;

        lv_obj_set_size(card, width, height);
        lv_obj_set_pos(card, x, y);
        
        // 💡 Fundo com cor viva/forte (Opacidade 100% Sólida)
        lv_obj_set_style_bg_color(card, card_color, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
        
        lv_obj_set_style_border_color(card, COLOR_BORDER_IDLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        // Título centralizado no topo do card
        lv_obj_t *lbl_title = lv_label_create(card);
        if (lbl_title) {
            lv_label_set_text(lbl_title, title);
            lv_obj_set_style_text_color(lbl_title, COLOR_TEXT_MUTED, LV_PART_MAIN);
            lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);
        }

        // Círculo / Arco Centralizado no meio do card
        lv_obj_t *arc = lv_arc_create(card);
        if (arc) {
            lv_obj_set_size(arc, 52, 52);
            lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);
            lv_arc_set_rotation(arc, 270);
            lv_arc_set_bg_angles(arc, 0, 360);
            lv_arc_set_range(arc, 0, 100);
            lv_arc_set_value(arc, 0);
            lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
            lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_arc_color(arc, COLOR_BG_MAIN, LV_PART_MAIN);
            lv_obj_set_style_arc_color(arc, arc_color, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);

            // Texto da Porcentagem/Valor no Centro exato do Círculo
            lv_obj_t *lbl_val = lv_label_create(arc);
            if (lbl_val) {
                lv_label_set_text(lbl_val, "--");
                lv_obj_set_style_text_color(lbl_val, COLOR_TEXT_WHITE, LV_PART_MAIN);
                lv_obj_center(lbl_val);
                if (out_label_val) *out_label_val = lbl_val;
            }

            if (out_arc) *out_arc = arc;
        }

        return card;
    }

public:
    DashboardUI() = default;

    // Inicialização do Layout no Display TFT
    void init() {
        lv_obj_t *scr = lv_screen_active();
        if (!scr) return;

        lv_obj_set_style_bg_color(scr, COLOR_BG_MAIN, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

        // --- TOPBAR ---
        lv_obj_t *topbar = lv_obj_create(scr);
        if (topbar) {
            lv_obj_set_size(topbar, 320, 32);
            lv_obj_set_pos(topbar, 0, 0);
            lv_obj_set_style_bg_color(topbar, COLOR_TOPBAR, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(topbar, COLOR_BORDER_IDLE, LV_PART_MAIN);
            lv_obj_set_style_border_side(topbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
            lv_obj_set_style_border_width(topbar, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(topbar, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(topbar, 4, LV_PART_MAIN);
            lv_obj_remove_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *lbl_title = lv_label_create(topbar);
            if (lbl_title) {
                lv_label_set_text(lbl_title, "DashBoard");
                lv_obj_set_style_text_color(lbl_title, COLOR_TEXT_WHITE, LV_PART_MAIN);
                lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);
            }

            label_host = lv_label_create(topbar);
            if (label_host) {
                lv_label_set_text(label_host, "Aguardando...");
                lv_obj_set_style_text_color(label_host, COLOR_TEXT_MUTED, LV_PART_MAIN);
                lv_obj_align(label_host, LV_ALIGN_CENTER, 10, 0);
            }

            label_status = lv_label_create(topbar);
            if (label_status) {
                lv_label_set_text(label_status, "STANDBY");
                lv_obj_set_style_text_color(label_status, COLOR_TEXT_MUTED, LV_PART_MAIN);
                lv_obj_align(label_status, LV_ALIGN_RIGHT_MID, -6, 0);
            }
        }

        const int card_w = 72;
        const int card_h = 120;
        const int card_y = 38;
        const int gap = 6;
        const int start_x = 6;

        // Construção dos 4 Cards Dinâmicos com Arcos Centralizados
        card_cpu  = create_metric_card(scr, start_x, card_y, card_w, card_h, COLOR_CARD_CPU, "CPU", COLOR_ARC_CPU, &arc_cpu, &label_cpu_val);
        card_ram  = create_metric_card(scr, start_x + (card_w + gap), card_y, card_w, card_h, COLOR_CARD_RAM, "RAM", COLOR_ARC_RAM, &arc_ram, &label_ram_val);
        card_disk = create_metric_card(scr, start_x + (card_w + gap) * 2, card_y, card_w, card_h, COLOR_CARD_DISK, "Disco", COLOR_ARC_DISK, &arc_disk, &label_disk_val);
        card_temp = create_metric_card(scr, start_x + (card_w + gap) * 3, card_y, card_w, card_h, COLOR_CARD_TEMP, "Temp", COLOR_ARC_TEMP, &arc_temp, &lbl_temp);

        // --- CARD OS / DETALHES ---
        card_os = lv_obj_create(scr);
        if (card_os) {
            lv_obj_set_size(card_os, 308, 68);
            lv_obj_set_pos(card_os, 6, 164);
            lv_obj_set_style_bg_color(card_os, COLOR_TOPBAR, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(card_os, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_color(card_os, COLOR_BORDER_IDLE, LV_PART_MAIN);
            lv_obj_set_style_border_width(card_os, 1, LV_PART_MAIN);
            lv_obj_set_style_radius(card_os, 8, LV_PART_MAIN);
            lv_obj_set_style_pad_all(card_os, 6, LV_PART_MAIN);
            lv_obj_remove_flag(card_os, LV_OBJ_FLAG_SCROLLABLE);

            lbl_os_name = lv_label_create(card_os);
            if (lbl_os_name) {
                lv_label_set_text(lbl_os_name, "SO: Sem Telemetria");
                lv_obj_set_style_text_color(lbl_os_name, COLOR_ARC_CPU, LV_PART_MAIN);
                lv_obj_align(lbl_os_name, LV_ALIGN_TOP_LEFT, 4, 2);
            }

            lbl_os_kernel = lv_label_create(card_os);
            if (lbl_os_kernel) {
                lv_label_set_text(lbl_os_kernel, "Kernel: N/A | Up: --");
                lv_obj_set_style_text_color(lbl_os_kernel, COLOR_TEXT_MUTED, LV_PART_MAIN);
                lv_obj_align(lbl_os_kernel, LV_ALIGN_TOP_LEFT, 4, 24);
            }
        }
    }

    // Atualização dos Valores nas Métricas
    void update_display(const char* host, const char* os_name, const char* kernel, 
                        float temp, float cpu_load, int ram_pct, int disk_pct, 
                        const char* uptime) {
        if (label_host && host) lv_label_set_text(label_host, host);
        if (lbl_os_name && os_name) lv_label_set_text_fmt(lbl_os_name, "SO: %s", os_name ? os_name : "N/A");
        if (lbl_os_kernel && kernel) lv_label_set_text_fmt(lbl_os_kernel, "Kernel: %s | Up: %s", 
                                                            kernel ? kernel : "N/A", 
                                                            uptime ? uptime : "--");

        // Cálculo Otimizado sem dependência de ponto flutuante pesado
        int cpu_pct = (int)((cpu_load * 100.0f) / 4.0f + 0.5f);
        if (cpu_pct > 100) cpu_pct = 100;
        if (cpu_pct < 0) cpu_pct = 0;

        if (arc_cpu) lv_arc_set_value(arc_cpu, cpu_pct);
        if (label_cpu_val) lv_label_set_text_fmt(label_cpu_val, "%d%%", cpu_pct);

        if (arc_ram) lv_arc_set_value(arc_ram, ram_pct);
        if (label_ram_val) lv_label_set_text_fmt(label_ram_val, "%d%%", ram_pct);

        if (arc_disk) lv_arc_set_value(arc_disk, disk_pct);
        if (label_disk_val) lv_label_set_text_fmt(label_disk_val, "%d%%", disk_pct);

        int temp_val = (int)(temp + 0.5f);
        int temp_arc_val = temp_val > 100 ? 100 : (temp_val < 0 ? 0 : temp_val);
        if (arc_temp) lv_arc_set_value(arc_temp, temp_arc_val);
        if (lbl_temp) lv_label_set_text_fmt(lbl_temp, "%d°C", temp_val);
    }

    // Alterna o Estilo / Nitidez das Cores conforme Estado do Servidor
    void set_online_status(bool online) {
        lv_color_t border_color = online ? COLOR_BORDER_ACTIVE : COLOR_BORDER_IDLE;
        
        // 💡 ALTERE AQUI PARA DEIXAR O FUNDO MAIS FORTE:
        // LV_OPA_COVER = 100% forte (cor pura do card)
        // LV_OPA_80    = 80% forte (muito visível)
        // LV_OPA_90    = 90% forte
        lv_opa_t card_opa = online ? LV_OPA_80 : LV_OPA_40;

        lv_obj_t* cards[] = {card_cpu, card_ram, card_disk, card_temp, card_os};
        for (lv_obj_t* card : cards) {
            if (card) {
                lv_obj_set_style_bg_opa(card, card_opa, LV_PART_MAIN);
                lv_obj_set_style_border_color(card, border_color, LV_PART_MAIN);
            }
        }

        if (label_status) {
            if (online) {
                lv_label_set_text(label_status, "ONLINE");
                lv_obj_set_style_text_color(label_status, COLOR_GREEN_ONLINE, LV_PART_MAIN);
            } else {
                lv_label_set_text(label_status, "OFFLINE");
                lv_obj_set_style_text_color(label_status, COLOR_ARC_TEMP, LV_PART_MAIN);
            }
        }
    }
};

// ==========================================
// INTERFACE DE COMPATIBILIDADE GLOBAL
// ==========================================
inline DashboardUI g_dashboard;

inline void create_dashboard_ui() {
    g_dashboard.init();
}

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