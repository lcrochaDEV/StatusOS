#ifndef HOURS_TIME_H
#define HOURS_TIME_H

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <Arduino.h>
#include <functional>
#include "ConsoleDependencies.h"

class Hours_Time {
  public:
    Hours_Time(const char* hours_sleep = "--:--", const char* hours_wakeon = "--:--", const char* date = "", long gmtOffset_sec = -3 * 3600, int daylightOffset_sec = 0, const char* ntpServer = "pool.ntp.org");
    ~Hours_Time();
    
    //METODO DE TODO O PROGRAMA
    void time_server();                   // Inicia o cliente NTP em segundo plano chamando
    void weke_on();                       // Verifica a hora atual e executa ações de ligar/desligar o display
    void manual_turn_on();                // Liga o display manualmente (no período noturno volta a dormir em 5 minutos)
    void manual_turn_off();               // Desliga o display manualmente e cancela o modo manual
    
    void monitorarConexao();              // Monitora a conexão Wi-Fi e imprime mensagens no Serial
    const char* losttime() const;         // Retorna a data e hora atual como string formatada (ex: "dd/mm/yyyy HH:MM:SS")

    // Getters
    const char* getHoursSleep() const;    // Retorna a hora de dormir como string (ex: "22:00")
    const char* getHoursWakeon() const;   // Retorna a hora de acordar como string (ex: "06:00")

    // Setters para atualização dinâmica via Web API
    void setHoursSleep(const String& sleep_time);
    void setHoursWakeon(const String& wake_time);

    // Ações disparadas nas transições de período (ex.: apagar/acender o backlight)
    void onSleep(std::function<void()> callback);
    void onWake(std::function<void()> callback);

    // Valida o formato "HH:MM" (00:00 a 23:59)
    static bool isValidTimeFormat(const char* hhmm);

    // Instância ativa (usada pelo servidor web para aplicar os horários vindos de /api/config)
    static Hours_Time* instance();

    // Controle de brilho usado ao dormir/acordar (padrão: g_file_system_ctrl)
    void setBrightnessControl(IBrightnessControl* brightness);
  private:
    // Configurações de Fuso Horário e NTP
    // Fuso horário de Brasília (GMT -3)
    String hours_sleep;              // hours_sleep: Deve ser o início do período noturno (22:00).
    String hours_wakeon;             // hours_wakeon: Deve ser o fim do período noturno (06:00).
    const char* date;                     // date: Armazena a data atual como string (ex: "dd/mm/yyyy").
    long gmtOffset_sec;                   // Offset do fuso horário em segundos (ex: -3 * 3600 para GMT-3).
    int daylightOffset_sec;               // 0 para não usar Horário de Verão
    const char* ntpServer;                // Servidor NTP para sincronização de tempo (ex: "pool.ntp.org").
    void calendar();                      // Atualiza a data e hora do sistema chamando o NTP 
    bool isNightPeriod(const char* hhmm) const;  // true se "HH:MM" está entre hours_sleep e hours_wakeon (inclusive virada de dia)
    void notificar(bool dormir);                 // Dispara o callback correspondente à transição

    std::function<void()> _onSleep = nullptr;
    std::function<void()> _onWake = nullptr;
    int _night_state = -1;                // -1 = desconhecido, 0 = dia, 1 = período noturno
    IBrightnessControl* _brightness = nullptr;
    uint8_t _brilho_antes_de_dormir = 80; // Brilho (%) memorizado ao dormir, restaurado ao acordar
    bool _display_apagado = false;        // true somente enquanto este módulo mantém o backlight apagado
    static Hours_Time* _instance;

    // Novo: Flag que indica se o display está em modo de timeout (ligado manualmente)
    bool is_manual_mode = false;          // Novo: Armazena a hora (em milissegundos) em que o display foi ligado manualmente
    // Novo: Armazena o tempo (em milissegundos) em que o display foi ligado manualmente
    unsigned long manual_on_timestamp = 0; // Novo: Armazena o tempo (em milissegundos) em que o display foi ligado manualmente
    // Novo: Constante para o tempo limite (5 minutos)
    const unsigned long TIMEOUT_MS = 5 * 60 * 1000;  // 5 minutos em milissegundos
};
 
#endif