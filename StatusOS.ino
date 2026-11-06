#include <Arduino.h>
#include "Display.h"
#include "ui_dashboard.h"
#include "ui_wol_dashboard.h"
#include "ui_crypto_dashboard.h"
#include "ui_screens.h"

// Inclusão dos módulos de rede e servidor do projeto
#include "WifiConnect.h"
#include "servidorweb.h"
#include "Hours_Time.h"

#include "Console.h"
Console console = Console("Mochi> ");

const char* hours_wakeon = "06:30";  // Fim do período noturno (06:30).
const char* hours_sleep = "22:03";   // Início do período noturno (22:03).


static void gesture_event_cb(lv_event_t * e) {
    lv_indev_t * indev = lv_indev_active();
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    lv_obj_t * act = lv_scr_act();
    if(dir == LV_DIR_LEFT) {
        if (act == g_screen_main) lv_scr_load_anim(g_screen_wol, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        else if (act == g_screen_wol) lv_scr_load_anim(g_screen_crypto, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        else lv_scr_load_anim(g_screen_main, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    } else if(dir == LV_DIR_RIGHT) {
        if (act == g_screen_main) lv_scr_load_anim(g_screen_crypto, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        else if (act == g_screen_crypto) lv_scr_load_anim(g_screen_wol, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        else lv_scr_load_anim(g_screen_main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

// Passa a referência da animação para o relógio
Hours_Time hours_Time_exec = Hours_Time(hours_sleep, hours_wakeon, "", -3 * 3600, 0, "pool.ntp.org");

#include <TFT_eSPI.h>
#include "Animations.h"

// 1. Instância ÚNICA do driver do hardware no .ino
TFT_eSPI tft = TFT_eSPI();

// 2. Injeção da MESMA instância tft para Display e Animations
Display display(tft);
Animations anim(tft);

/* Credenciais Wi-Fi */
const char* SSID = "PERIGO";
const char* PASSWORD = "LIBER@RWIFI_";

WifiConnect wifiConnect(SSID, PASSWORD);

// Servidor web só fica ativo com a rede conectada; sem ela (ou com o Portal AP aberto) permanece em standby
bool atualizarServidorWeb() {
    static bool rotasRegistradas = false;
    static bool servidorAtivo = false;
    static uint32_t inicioServidorMs = 0;
    static uint32_t ultimaTentativaBindMs = 0;
    const uint32_t intervaloBindMs = 1000;
    const uint32_t janelaBindMs = 180000;

    bool conectado = (wifiConnect.getState() == WifiState::Connected) && (WiFi.status() == WL_CONNECTED);

    if (conectado && !servidorAtivo) {
        if (!rotasRegistradas) {
            setup_web_server();   // Registra as rotas e inicia (server.begin()) uma única vez
            rotasRegistradas = true;
        } else {
            server.begin();       // Retoma o servidor já configurado
        }
        servidorAtivo = true;
        inicioServidorMs = millis();
        ultimaTentativaBindMs = millis();
        Serial.printf("[StatusOS] Servidor Web ativo em http://%s\n", WifiConnect::obtainIP());
    } else if (conectado && servidorAtivo && millis() - inicioServidorMs < janelaBindMs && millis() - ultimaTentativaBindMs >= intervaloBindMs) {
        // Após um portal sem configuração, a porta 80 pode demorar a liberar no lwIP (TIME_WAIT):
        // o begin() é repetido na janela e não faz nada quando o servidor já está escutando
        ultimaTentativaBindMs = millis();
        server.begin();
    } else if (!conectado && servidorAtivo) {
        ws.closeAll();
        server.end();             // Libera a porta 80 para o Portal AP
        servidorAtivo = false;
    }
    return servidorAtivo;
}

// Acompanha mudanças de conexão após o boot e sincroniza a interface
void atualizarStatusRede() {
    static bool online = (wifiConnect.getState() == WifiState::Connected) && (WiFi.status() == WL_CONNECTED);

    bool conectado = (wifiConnect.getState() == WifiState::Connected) && (WiFi.status() == WL_CONNECTED);

    if (conectado && !online) {
        ui_fechar_popup_aviso_rede();
        hours_Time_exec.time_server();
        update_status(true);
        update_wol_status(true); // Atualiza tela WoL
        online = true;
    } else if (!conectado && online) {
        update_status(false);
        update_wol_status(false); // Atualiza tela WoL
        online = false;
    }
    check_wol_sd_status(); // Verifica SD em tempo real
    check_crypto_sd_status();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando o sistema de StatusOS de forma assíncrona...");

    // -------------------------------------------------------------------------
    // PASSO 1: Inicializa o Display, Backlight e o motor LVGL (via Classe Display)
    // -------------------------------------------------------------------------
    if (!display.begin()) {
        Serial.println("[ERRO CRÍTICO] Falha ao inicializar o Display / LVGL.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Non-blocking safe trap para o WDT
        }
    }
    
    // Força o primeiro processamento de timer para instanciar o contexto gráfico do LVGL
    lv_timer_handler();

    // -------------------------------------------------------------------------
    // PASSO 2: Inicialização do módulo de Animações e Console
    // -------------------------------------------------------------------------
    anim.begin();
    console.setAnimations(&anim);

    // Callback de redesenho seguro do LVGL ao encerrar animações
    anim.onAnimationEnd([]() {
        lv_obj_t* active_screen = lv_scr_act();
        if (active_screen != nullptr) {
            lv_obj_invalidate(active_screen);
        }
    });

    // -------------------------------------------------------------------------
    // PASSO 3: Controle de Fluxo Baseado no Status de Conexão (Sem Bloqueio)
    // -------------------------------------------------------------------------
    // Conexão manual com as credenciais armazenadas (A.P. só se nunca conectou ou credenciais mudaram)
    wifiConnect.begin();

    // Tela de boot (conectado ou não): encerra com o Wi-Fi conectado ou pelo tempo limite, tratado em anim.update()
    anim.playAnimation(GifID::BOOT);
    while (wifiConnect.getState() == WifiState::Connecting) {
        wifiConnect.loop();
        if (anim.isPlaying()) anim.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (!wifiConnect.checkStatus()) {
        Serial.println("[StatusOS] Sem conexão. Exibindo Dashboard, Pop-up de Aviso e Animação Offline...");

        // Constrói o dashboard base, abre o pop-up de aviso de rede e define o status offline na topbar
        g_screen_main = lv_screen_active();
        create_dashboard_ui();
        ui_abrir_popup_aviso_rede();
        update_status(false);

        g_screen_wol = lv_obj_create(NULL);
        create_wol_screen(g_screen_wol);

        g_screen_crypto = lv_obj_create(NULL);
        create_crypto_screen(g_screen_crypto);

        lv_obj_add_event_cb(g_screen_main, gesture_event_cb, LV_EVENT_GESTURE, NULL);
        lv_obj_add_event_cb(g_screen_wol, gesture_event_cb, LV_EVENT_GESTURE, NULL);
        lv_obj_add_event_cb(g_screen_crypto, gesture_event_cb, LV_EVENT_GESTURE, NULL);

        // Dispara a animação offline de forma assíncrona
        //anim.playAnimation(GifID::NOT_WIFI);
    } else {
        Serial.println("[StatusOS] Conectado! Sincronizando NTP e iniciando serviços...");

        hours_Time_exec.time_server();

        g_screen_main = lv_screen_active();
        create_dashboard_ui();

        g_screen_wol = lv_obj_create(NULL);
        create_wol_screen(g_screen_wol);

        g_screen_crypto = lv_obj_create(NULL);
        create_crypto_screen(g_screen_crypto);

        lv_obj_add_event_cb(g_screen_main, gesture_event_cb, LV_EVENT_GESTURE, NULL);
        lv_obj_add_event_cb(g_screen_wol, gesture_event_cb, LV_EVENT_GESTURE, NULL);
        lv_obj_add_event_cb(g_screen_crypto, gesture_event_cb, LV_EVENT_GESTURE, NULL);

        update_status(true);
    }

    console.helloWord();        
}
void loop() {
    wifiConnect.loop();
    atualizarStatusRede();

    // Alternância: se a animação estiver tocando, executa a atualização do GIF.
    // Assim que parar, retoma o Dashboard (LVGL).
    (anim.isPlaying()) ? anim.update() : display.update();

    // Processa as requisições do Servidor Web e WebSocket
    if (atualizarServidorWeb()) process_web_server_tasks();

    // Atualiza a tela de Cripto
    crypto_loop();

    // Cede tempo para o sistema operacional / Watchdog do ESP32
    vTaskDelay(pdMS_TO_TICKS(5));

    hours_Time_exec.weke_on();
    console.consoleView();
}