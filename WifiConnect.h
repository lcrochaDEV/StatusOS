#ifndef WIFICONNECT_H
#define WIFICONNECT_H

#include <Arduino.h>

class WifiConnect {
  public:
    WifiConnect(const char* ssid = nullptr, const char* password = nullptr);
    
    // Métodos padronizados (camelCase)
    void connectManual();
    void startAccessPoint();
    bool checkStatus();
    
  private:
    const char* ssid; 
    const char* password;
    int maxTentativas = 20;
    int tentativaAtual = 0;
};

#endif // WIFICONNECT_H