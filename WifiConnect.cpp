// Configuração automática de bibliotecas baseada na placa
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <WiFiManager.h>
#include "WifiConnect.h"

WifiConnect::WifiConnect(const char* ssid, const char* password)
  : ssid(ssid), password(password) {
}

void WifiConnect::connectManual() {
  if (ssid == nullptr) {
    Serial.println("[Wi-Fi] Erro: SSID não fornecido para conexão manual.");
    return;
  }

  // 1. Limpeza e restauração total do rádio
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.printf("[Wi-Fi] Conectando a %s ", ssid);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  tentativaAtual = 0;

  while (WiFi.status() != WL_CONNECTED) {
    if (tentativaAtual >= maxTentativas) {
      Serial.println("\n[Wi-Fi] Falha ao conectar!");
      return;
    }
    
    delay(500);
    Serial.print(".");
    tentativaAtual++;
  }

  checkStatus();
}

void WifiConnect::startAccessPoint() {
  WiFiManager wm; 

  // 1. Liga o rádio no modo misto para permitir a inicialização do hardware
  WiFi.mode(WIFI_AP_STA);

  // 2. Aguarda a carga do driver até que o endereço MAC físico possa ser lido
  uint8_t tentativasEstabilizacao = 0;
  while ((WiFi.macAddress() == "00:00:00:00:00:00" || WiFi.macAddress().length() == 0) && tentativasEstabilizacao < 10) {
    delay(100);
    tentativasEstabilizacao++;
  }

  if (WiFi.macAddress() == "00:00:00:00:00:00") {
    Serial.println("[Wi-Fi] Erro Crítico: Rádio Wi-Fi não estabilizou a tempo.");
    return;
  }

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  wm.setDebugOutput(false); 

  String macLimpo = WiFi.macAddress();
  macLimpo.replace(":", "");
  String ssidFinal = "ESP:" + macLimpo;

  if (!wm.autoConnect(ssidFinal.c_str(), "senha123")) { 
    Serial.println("[Wi-Fi] Falha na conexão ou tempo esgotado via Portal."); 
    return;
  } 

  // 3. Encerra a interface SoftAP e libera a porta 80 do socket lwIP para o servidor AsyncWebServer
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  checkStatus();
}

bool WifiConnect::checkStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[Wi-Fi] Conectado à Rede: %s\n", WiFi.SSID().c_str()); 
    Serial.printf("[Wi-Fi] Endereço IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[Wi-Fi] Endereço MAC: %s\n", WiFi.macAddress().c_str()); 
    Serial.printf("[Wi-Fi] Canal Wi-Fi atual: %d\n\n", WiFi.channel()); 
    return true;
  } else {
    Serial.println("\n[Wi-Fi] Wi-Fi Desconectado!");
    return false;
  }
}