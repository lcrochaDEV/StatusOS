#ifndef WIFICONNECT_H
#define WIFICONNECT_H

#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_mac.h>
#endif

// Estados da máquina de conexão assíncrona
enum class WifiState {
  Disconnected,
  Connecting,
  Connected,
  Failure
};

class WifiConnect {
  public:
    WifiConnect(const char* ssid = nullptr, const char* password = nullptr);
    
    // Configuração e Conexão Inicial
    void connectManual();
    void startAccessPoint(const char* apPassword = "senha123", uint32_t timeoutSegundos = 180);
    
    // Máquina de Estados Não Bloqueante (Deve ser chamada periodicamente no loop principal)
    void loop();

    // Métodos de Checagem e Status
    bool checkStatus();
    WifiState getState() const;

    // Getters Globais (Zero Heap / Seguros para a UI)
    static const char* obtainIP();
    static const char* obtainMascara();
    static const char* obtainMacAddress();

  private:
    const char* ssid; 
    const char* password;
    
    WifiState estadoAtual{WifiState::Disconnected};
    
    // Controle Assíncrono por Tempo (Não Bloqueante)
    uint32_t tempoInicioConexao = 0;
    const uint32_t timeoutConexaoMs = 15000; // 15 segundos para dar Timeout manual
    
    uint32_t ultimaTentativaReconexao = 0;
    const uint32_t intervaloReconexaoMs = 10000; // Tenta reconectar a cada 10s se cair

    void extrairMacHardware(char* bufferOut, size_t bufferSize);
};

#endif // WIFICONNECT_H