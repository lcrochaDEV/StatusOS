#ifndef WIFICONNECT_H
#define WIFICONNECT_H

#include <Arduino.h>
#include "SecurityConfig.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_mac.h>
#endif

enum class WifiState {
  Disconnected,
  Connecting,
  Connected,
  Failure
};

class WifiConnect {
  public:
    WifiConnect(const char* ssid = nullptr, const char* password = nullptr);
    
    void begin();
    void connectManual();
    void startAccessPoint(const char* apPassword = AP_PASSWORD_DEFAULT, uint32_t timeoutSegundos = 180);
    void clearCredentials(); // Limpa as credenciais salvas via console/botão e abre o AP
    
    void loop();

    bool checkStatus();
    WifiState getState() const;

    static const char* obtainIP();
    static const char* obtainMascara();
    static const char* obtainMacAddress();

  private:
    const char* ssid; 
    const char* password;

    char ssidBuffer[33] = {0};
    char passwordBuffer[65] = {0};
    
    WifiState estadoAtual{WifiState::Disconnected};
    
    uint32_t tempoInicioConexao = 0;
    const uint32_t timeoutConexaoMs = 15000;
    
    uint32_t ultimaTentativaReconexao = 0;
    const uint32_t intervaloReconexaoMs = 10000;

    bool credenciaisConfirmadas = false;
    uint8_t falhasCredencial = 0;
    const uint8_t limiteFalhasCredencial = 3;

    void extrairMacHardware(char* bufferOut, size_t bufferSize);
    bool possuiCredenciais() const;
    bool carregarCredenciais();
    void salvarCredenciais();
    void avaliarFalhaDeConexao();
};

#endif // WIFICONNECT_H