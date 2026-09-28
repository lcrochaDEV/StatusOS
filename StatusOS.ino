#include <Arduino.h>
#include "Display.h"
#include "ui_dashboard.h"

// Inclusão dos módulos de rede e servidor do projeto
#include "WifiConnect.h"
#include "servidorweb.h"
#include "Hours_Time.h"

#include "Console.h"
Console console = Console("Mochi> ");

const char* hours_wakeon = "06:30";  // hours_down: Deve ser o fim do período noturno (06:00).
const char* hours_sleep = "22:03";   // hours_up: Deve ser o início do período noturno (22:00).
#include "Hours_Time.h"

// Passa a referência da animação para o relógio
Hours_Time hours_Time_exec = Hours_Time(hours_sleep, hours_wakeon, "", -3 * 3600, 0, "pool.ntp.org");

// Instância global do gerenciador de display e LVGL v9[cite: 11]
Display display;

/* Credenciais Wi-Fi */
const char* SSID = "PERIGO";
const char* PASSWORD = "LIBER@RWIFI";

WifiConnect wifiConnect = WifiConnect(SSID, PASSWORD);

void startWifi() {
    wifiConnect.startAccessPoint();
}


void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando o sistema de StatusOS...");

    startWifi(); 
    hours_Time_exec.time_server();                            

    // 1. Inicializa o Display primeiro
    if (!display.begin()) {
        Serial.println("[ERRO] Sistema travado devido a falha no Display.");
        while (1) { delay(1000); }
    }
    
    create_dashboard_ui(); // 1. Constrói a interface do Dashboard no LVGL
    setup_web_server();    // 2. Inicia o servidor web                                      
    console.helloWord();   // 3. Boas vindas        
}

void loop() {
    display.update();

    process_web_server_tasks();
    vTaskDelay(pdMS_TO_TICKS(5)); 

    hours_Time_exec.weke_on();
    console.consoleView();
}