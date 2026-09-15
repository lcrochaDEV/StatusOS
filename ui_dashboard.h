#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <lvgl.h>
#include <stdio.h>
#include <math.h>

// ==========================================
// PALETA DE CORES - DARK MODE NEON
// ==========================================
#define COLOR_BG_MAIN       lv_color_hex(0x0A0B10) // Fundo Preto Grafite
#define COLOR_TOPBAR        lv_color_hex(0x12131C) // Topbar Escura
#define COLOR_BORDER_IDLE   lv_color_hex(0x222436) // Borda em Standby
#define COLOR_BORDER_ACTIVE lv_color_hex(0x3D426B) // Borda Ativa (Mais Nítida)

// Cartões Escuros (Dark Cards)
#define COLOR_CARD_CPU      lv_color_hex(0x13172E) 
#define COLOR_ARC_CPU       lv_color_hex(0x00F2FE) 

#define COLOR_CARD_RAM      lv_color_hex(0x122428) 
#define COLOR_ARC_RAM       lv_color_hex(0xff007f) 

#define COLOR_CARD_DISK     lv_color_hex(0x23142B) 
#define COLOR_ARC_DISK      lv_color_hex(0xBD00FF) 

#define COLOR_CARD_TEMP     lv_color_hex(0x2B121C) 
#define COLOR_ARC_TEMP      lv_color_hex(0xFF0055) 

#define COLOR_TEXT_WHITE    lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_MUTED    lv_color_hex(0x8F94B8)
#define COLOR_GREEN_ONLINE  lv_color_hex(0x00FF88)

// ==========================================
// PONTEIROS GLOBAIS DE CONTROLE
// ==========================================
inline lv_obj_t *label_host = NULL;
inline lv_obj_t *label_status = NULL;

// Ponteiros dos Cartões (para controle de estilo via JSON)
inline lv_obj_t *card_cpu = NULL;
inline lv_obj_t *card_ram = NULL;
inline lv_obj_t *card_disk = NULL;
inline lv_obj_t *card_temp = NULL;
inline lv_obj_t *card_os = NULL;

inline lv_obj_t *arc_cpu = NULL;
inline lv_obj_t *label_cpu_val = NULL;

inline lv_obj_t *arc_ram = NULL;
inline lv_obj_t *label_ram_val = NULL;

inline lv_obj_t *arc_disk = NULL;
inline lv_obj_t *label_disk_val = NULL;

inline lv_obj_t *arc_temp = NULL;
inline lv_obj_t *lbl_temp = NULL;

inline lv_obj_t *lbl_os_name = NULL;
inline lv_obj_t *lbl_os_kernel = NULL;

// ==========================================
// FUNÇÃO DE ATUALIZAÇÃO VIA JSON
// ==========================================
inline void update_telemetry_data(const char* host, const char* os_name, const char* kernel, 
                               float temp, float cpu_load, int ram_pct, int disk_pct, 
                               const char* uptime, const void* logo_url) {
    
    // --- 1. AUMENTA A NITIDEZ/OPACIDADE DOS CARDS AO RECEBER O JSON ---
    if (card_cpu) {
        lv_obj_set_style_bg_opa(card_cpu, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_cpu, COLOR_BORDER_ACTIVE, LV_PART_MAIN);
    }
    if (card_ram) {
        lv_obj_set_style_bg_opa(card_ram, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_ram, COLOR_BORDER_ACTIVE, LV_PART_MAIN);
    }
    if (card_disk) {
        lv_obj_set_style_bg_opa(card_disk, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_disk, COLOR_BORDER_ACTIVE, LV_PART_MAIN);
    }
    if (card_temp) {
        lv_obj_set_style_bg_opa(card_temp, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_temp, COLOR_BORDER_ACTIVE, LV_PART_MAIN);
    }
    if (card_os) {
        lv_obj_set_style_bg_opa(card_os, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_os, COLOR_BORDER_ACTIVE, LV_PART_MAIN);
    }

    // --- 3. ATUALIZAÇÃO DOS VALORES E TEXTOS ---
    if (label_host && host) lv_label_set_text(label_host, host);
    if (lbl_os_name && os_name) lv_label_set_text_fmt(lbl_os_name, "SO: %s", os_name);
    if (lbl_os_kernel && kernel) lv_label_set_text_fmt(lbl_os_kernel, "Kernel: %s | Up: %s", kernel, uptime ? uptime : "--");

    int cpu_pct = (int)fminf(roundf((cpu_load * 100.0f) / 4.0f), 100.0f);
    if (arc_cpu) lv_arc_set_value(arc_cpu, cpu_pct);
    if (label_cpu_val) lv_label_set_text_fmt(label_cpu_val, "%d%%", cpu_pct);

    if (arc_ram) lv_arc_set_value(arc_ram, ram_pct);
    if (label_ram_val) lv_label_set_text_fmt(label_ram_val, "%02d%%", ram_pct);

    if (arc_disk) lv_arc_set_value(arc_disk, disk_pct);
    if (label_disk_val) lv_label_set_text_fmt(label_disk_val, "%02d%%", disk_pct);

    int temp_val = (int)temp;
    if (arc_temp) lv_arc_set_value(arc_temp, temp_val > 100 ? 100 : temp_val);
    if (lbl_temp) lv_label_set_text_fmt(lbl_temp, "%d°C", (int)roundf(temp));
}

inline void update_status(bool online) {
    if (label_status) {
        if (online) {
            lv_label_set_text(label_status, "ONLINE");
            lv_obj_set_style_text_color(label_status, COLOR_GREEN_ONLINE, LV_PART_MAIN);
        } else {
            lv_label_set_text(label_status, "OFFLINE");
            lv_obj_set_style_text_color(label_status, COLOR_ARC_TEMP, LV_PART_MAIN);
            
            if (card_cpu) lv_obj_set_style_bg_opa(card_cpu, LV_OPA_30, LV_PART_MAIN);
            if (card_ram) lv_obj_set_style_bg_opa(card_ram, LV_OPA_30, LV_PART_MAIN);
            if (card_disk) lv_obj_set_style_bg_opa(card_disk, LV_OPA_30, LV_PART_MAIN);
            if (card_temp) lv_obj_set_style_bg_opa(card_temp, LV_OPA_30, LV_PART_MAIN);
            if (card_os) lv_obj_set_style_bg_opa(card_os, LV_OPA_30, LV_PART_MAIN);
        }
    }
}

// ==========================================
// CONSTRUÇÃO DA INTERFACE LVGL
// ==========================================
inline void create_dashboard_ui() {
    lv_obj_t *scr = lv_screen_active();
    
    // Fundo Principal Dark
    lv_obj_set_style_bg_color(scr, COLOR_BG_MAIN, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // --- TOPBAR ---
    lv_obj_t *topbar = lv_obj_create(scr);
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
    lv_label_set_text(lbl_title, "DashBoard");
    lv_obj_set_style_text_color(lbl_title, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);

    label_host = lv_label_create(topbar);
    lv_label_set_text(label_host, "Aguardando...");
    lv_obj_set_style_text_color(label_host, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(label_host, LV_ALIGN_CENTER, 10, 0);

    label_status = lv_label_create(topbar);
    lv_label_set_text(label_status, "STANDBY");
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_RIGHT_MID, -6, 0);

    const int card_w = 72;
    const int card_h = 120;
    const int card_y = 38;
    const int gap = 6;
    const int start_x = 6;

    // --- CARD 1: CPU ---
    card_cpu = lv_obj_create(scr);
    lv_obj_set_size(card_cpu, card_w, card_h);
    lv_obj_set_pos(card_cpu, start_x, card_y);
    lv_obj_set_style_bg_color(card_cpu, COLOR_CARD_CPU, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card_cpu, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_cpu, COLOR_BORDER_IDLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_cpu, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_cpu, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_cpu, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card_cpu, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t1 = lv_label_create(card_cpu);
    lv_label_set_text(t1, "CPU Load");
    lv_obj_set_style_text_color(t1, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 2, 2);

    label_cpu_val = lv_label_create(card_cpu);
    lv_label_set_text(label_cpu_val, "--");
    lv_obj_set_style_text_color(label_cpu_val, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(label_cpu_val, LV_ALIGN_LEFT_MID, 2, -10);

    lv_obj_t *sub1 = lv_label_create(card_cpu);
    lv_label_set_text(sub1, "de 100%");
    lv_obj_set_style_text_color(sub1, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(sub1, LV_ALIGN_LEFT_MID, 2, 8);

    arc_cpu = lv_arc_create(card_cpu);
    lv_obj_set_size(arc_cpu, 38, 38);
    lv_obj_align(arc_cpu, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_arc_set_rotation(arc_cpu, 270);
    lv_arc_set_bg_angles(arc_cpu, 0, 360);
    lv_arc_set_range(arc_cpu, 0, 100);
    lv_arc_set_value(arc_cpu, 0);
    lv_obj_remove_style(arc_cpu, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_cpu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_cpu, COLOR_BG_MAIN, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_cpu, COLOR_ARC_CPU, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_cpu, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_cpu, 5, LV_PART_INDICATOR);

    // --- CARD 2: RAM ---
    card_ram = lv_obj_create(scr);
    lv_obj_set_size(card_ram, card_w, card_h);
    lv_obj_set_pos(card_ram, start_x + (card_w + gap), card_y);
    lv_obj_set_style_bg_color(card_ram, COLOR_CARD_RAM, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card_ram, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_ram, COLOR_BORDER_IDLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_ram, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_ram, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_ram, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card_ram, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t2 = lv_label_create(card_ram);
    lv_label_set_text(t2, "RAM Uso");
    lv_obj_set_style_text_color(t2, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(t2, LV_ALIGN_TOP_LEFT, 2, 2);

    label_ram_val = lv_label_create(card_ram);
    lv_label_set_text(label_ram_val, "--");
    lv_obj_set_style_text_color(label_ram_val, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(label_ram_val, LV_ALIGN_LEFT_MID, 2, -10);

    lv_obj_t *sub2 = lv_label_create(card_ram);
    lv_label_set_text(sub2, "de 100%");
    lv_obj_set_style_text_color(sub2, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(sub2, LV_ALIGN_LEFT_MID, 2, 8);

    arc_ram = lv_arc_create(card_ram);
    lv_obj_set_size(arc_ram, 38, 38);
    lv_obj_align(arc_ram, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_arc_set_rotation(arc_ram, 270);
    lv_arc_set_bg_angles(arc_ram, 0, 360);
    lv_arc_set_range(arc_ram, 0, 100);
    lv_arc_set_value(arc_ram, 0);
    lv_obj_remove_style(arc_ram, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_ram, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_ram, COLOR_BG_MAIN, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_ram, COLOR_ARC_RAM, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_ram, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_ram, 5, LV_PART_INDICATOR);

    // --- CARD 3: DISCO ---
    card_disk = lv_obj_create(scr);
    lv_obj_set_size(card_disk, card_w, card_h);
    lv_obj_set_pos(card_disk, start_x + (card_w + gap) * 2, card_y);
    lv_obj_set_style_bg_color(card_disk, COLOR_CARD_DISK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card_disk, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_disk, COLOR_BORDER_IDLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_disk, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_disk, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_disk, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card_disk, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t3 = lv_label_create(card_disk);
    lv_label_set_text(t3, "Disco");
    lv_obj_set_style_text_color(t3, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(t3, LV_ALIGN_TOP_LEFT, 2, 2);

    label_disk_val = lv_label_create(card_disk);
    lv_label_set_text(label_disk_val, "--");
    lv_obj_set_style_text_color(label_disk_val, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(label_disk_val, LV_ALIGN_LEFT_MID, 2, -10);

    lv_obj_t *sub3 = lv_label_create(card_disk);
    lv_label_set_text(sub3, "de 100%");
    lv_obj_set_style_text_color(sub3, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(sub3, LV_ALIGN_LEFT_MID, 2, 8);

    arc_disk = lv_arc_create(card_disk);
    lv_obj_set_size(arc_disk, 38, 38);
    lv_obj_align(arc_disk, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_arc_set_rotation(arc_disk, 270);
    lv_arc_set_bg_angles(arc_disk, 0, 360);
    lv_arc_set_range(arc_disk, 0, 100);
    lv_arc_set_value(arc_disk, 0);
    lv_obj_remove_style(arc_disk, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_disk, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_disk, COLOR_BG_MAIN, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_disk, COLOR_ARC_DISK, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_disk, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_disk, 5, LV_PART_INDICATOR);

    // --- CARD 4: TEMPERATURA ---
    card_temp = lv_obj_create(scr);
    lv_obj_set_size(card_temp, card_w, card_h);
    lv_obj_set_pos(card_temp, start_x + (card_w + gap) * 3, card_y);
    lv_obj_set_style_bg_color(card_temp, COLOR_CARD_TEMP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card_temp, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_temp, COLOR_BORDER_IDLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_temp, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_temp, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_temp, 4, LV_PART_MAIN);
    lv_obj_remove_flag(card_temp, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t4 = lv_label_create(card_temp);
    lv_label_set_text(t4, "Temp CPU");
    lv_obj_set_style_text_color(t4, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(t4, LV_ALIGN_TOP_LEFT, 2, 2);

    lbl_temp = lv_label_create(card_temp);
    lv_label_set_text(lbl_temp, "--");
    lv_obj_set_style_text_color(lbl_temp, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_align(lbl_temp, LV_ALIGN_LEFT_MID, 2, -10);

    lv_obj_t *sub4 = lv_label_create(card_temp);
    lv_label_set_text(sub4, "de 100°C");
    lv_obj_set_style_text_color(sub4, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(sub4, LV_ALIGN_LEFT_MID, 2, 8);

    arc_temp = lv_arc_create(card_temp);
    lv_obj_set_size(arc_temp, 38, 38);
    lv_obj_align(arc_temp, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_arc_set_rotation(arc_temp, 270);
    lv_arc_set_bg_angles(arc_temp, 0, 360);
    lv_arc_set_range(arc_temp, 0, 100);
    lv_arc_set_value(arc_temp, 0);
    lv_obj_remove_style(arc_temp, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_temp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_temp, COLOR_BG_MAIN, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_temp, COLOR_ARC_TEMP, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_temp, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_temp, 5, LV_PART_INDICATOR);

    // --- CARD OS / DETALHES ---
    card_os = lv_obj_create(scr);
    lv_obj_set_size(card_os, 308, 68);
    lv_obj_set_pos(card_os, 6, 164);
    lv_obj_set_style_bg_color(card_os, COLOR_TOPBAR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card_os, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(card_os, COLOR_BORDER_IDLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card_os, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_os, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_os, 6, LV_PART_MAIN);
    lv_obj_remove_flag(card_os, LV_OBJ_FLAG_SCROLLABLE);

    lbl_os_name = lv_label_create(card_os);
    lv_label_set_text(lbl_os_name, "SO: Sem Telemetria");
    lv_obj_set_style_text_color(lbl_os_name, COLOR_ARC_CPU, LV_PART_MAIN);
    lv_obj_align(lbl_os_name, LV_ALIGN_TOP_LEFT, 4, 2);

    lbl_os_kernel = lv_label_create(card_os);
    lv_label_set_text(lbl_os_kernel, "Kernel: N/A | Up: --");
    lv_obj_set_style_text_color(lbl_os_kernel, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_align(lbl_os_kernel, LV_ALIGN_TOP_LEFT, 4, 24);
}

#endif // UI_DASHBOARD_H