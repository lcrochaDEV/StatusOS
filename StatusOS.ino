#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "ui_dashboard.h"
#include "servidorweb.h"
#include "WifiConnect.h"

/* Instância do Hardware TFT */
TFT_eSPI tft = TFT_eSPI();

/* Buffer do LVGL v9 (Alocado dinamicamente na RAM Heap para não estourar a .bss) */
#define DRAW_BUF_SIZE (320 * 10 * sizeof(uint16_t))
static uint8_t *draw_buf = NULL;

/* Credenciais Wi-Fi */
const char* SSID = "PERIGO";
const char* PASSWORD = "LIBER@RWIFI";

WifiConnect wifiConnect = WifiConnect(SSID, PASSWORD);

/* Callback de renderização do LVGL */
IRAM_ATTR void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void startWifi() {
    wifiConnect.connections_Wifi();
}

void setup() {
    Serial.begin(115200);

    startWifi(); // 1. Conexão Wi-Fi

    // 2. Inicialização do Hardware do Display TFT
    tft.init();
    tft.setRotation(1);
    
    // Mude de true para false para restaurar o fundo preto e os cartões escuros
    tft.invertDisplay(false); 

    // 3. Alocação Dinâmica de Memória para o LVGL
    draw_buf = (uint8_t *)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (draw_buf == NULL) {
        Serial.println("ERRO: Falha ao alocar memória Heap para o LVGL!");
        return;
    }

    // 4. Inicialização do LVGL v9 e Registro do Display
    lv_init();

    lv_display_t *disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 5. Interface Gráfica e Servidor Web
    create_dashboard_ui();
    setup_web_server();
}
void loop() {
    static uint32_t last_tick = millis();
    uint32_t current_time = millis();
    
    // Atualiza o contador de ticks do LVGL com o tempo decorrido
    lv_tick_inc(current_time - last_tick);
    last_tick = current_time;

    // Executa as tarefas do LVGL
    lv_timer_handler();

    // Executa a máquina de estados do Wi-Fi
    wifiConnect.loop();

    // Processa o servidor web e WebSockets
    process_web_server_tasks();

    vTaskDelay(pdMS_TO_TICKS(5)); // Evita o uso excessivo de CPU
}