#include "WifiConnect.h"
#include <WiFiManager.h>

WifiConnect::WifiConnect(const char* ssid, const char* password)
  : ssid(ssid), password(password) {
}

// Extrai o MAC do Hardware diretamente (Imune a falhas de inicialização do rádio)
void WifiConnect::extrairMacHardware(char* bufferOut, size_t bufferSize) {
#if defined(ESP32)
    uint8_t macBase[6];
    esp_efuse_mac_get_default(macBase);
    snprintf(bufferOut, bufferSize, "%02X%02X%02X%02X%02X%02X", 
             macBase[0], macBase[1], macBase[2], macBase[3], macBase[4], macBase[5]);
#else
    String macLimpo = WiFi.macAddress();
    macLimpo.replace(":", "");
    snprintf(bufferOut, bufferSize, "%s", macLimpo.c_str());
#endif
}

// Inicializa a tentativa de conexão manual de forma ASSÍNCRONA
void WifiConnect::connectManual() {
    if (ssid == nullptr || strlen(ssid) == 0) {
        Serial.println("[Wi-Fi] Erro: SSID não fornecido.");
        estadoAtual = WifiState::Failure;
        return;
    }

    Serial.printf("[Wi-Fi] Iniciando conexão assíncrona a: %s\n", ssid);
    
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    tempoInicioConexao = millis();
    estadoAtual = WifiState::Connecting;
}

// Processamento Não Bloqueante executado continuamente no loop()
void WifiConnect::loop() {
    // 1. Monitora o processo de Conexão Ativa
    if (estadoAtual == WifiState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            estadoAtual = WifiState::Connected;
            checkStatus();
        } else if (millis() - tempoInicioConexao >= timeoutConexaoMs) {
            Serial.println("\n[Wi-Fi] Tempo limite de conexão esgotado!");
            estadoAtual = WifiState::Failure;
            ultimaTentativaReconexao = millis();
        }
    } 
    // 2. Reconexão Automática Passiva (Sem travar o processador)
    else if (estadoAtual == WifiState::Failure || (estadoAtual == WifiState::Connected && WiFi.status() != WL_CONNECTED)) {
        if (millis() - ultimaTentativaReconexao >= intervaloReconexaoMs) {
            Serial.println("[Wi-Fi] Tentando reconectar à rede...");
            connectManual();
        }
    }
}

// Portal de Configuração com liberação de recursos
void WifiConnect::startAccessPoint(const char* apPassword, uint32_t timeoutSegundos) {
    WiFiManager wm; 

    // Configura o portal de forma profissional
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(timeoutSegundos); // Não fica travado para sempre se ninguém conectar

    char macStr[13];
    extrairMacHardware(macStr, sizeof(macStr));
    
    char ssidFinal[32];
    snprintf(ssidFinal, sizeof(ssidFinal), "ESP_%s", macStr);

    Serial.printf("[Wi-Fi] Abrindo Portal AP: %s\n", ssidFinal);

    WiFi.persistent(false);

    if (!wm.autoConnect(ssidFinal, apPassword)) { 
        Serial.println("[Wi-Fi] Falha na conexão ou tempo limite atingido no Portal."); 
        estadoAtual = WifiState::Failure;
    } else {
        estadoAtual = WifiState::Connected;
        checkStatus();
    }

    // Libera a porta 80 e encerra totalmente a interface de AP para o AsyncWebServer
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

bool WifiConnect::checkStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[Wi-Fi] Connected à Rede: %s\n", WiFi.SSID().c_str()); 
        Serial.printf("[Wi-Fi] Endereço IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[Wi-Fi] Endereço MAC: %s\n", WiFi.macAddress().c_str()); 
        Serial.printf("[Wi-Fi] Canal Wi-Fi: %d\n\n", WiFi.channel()); 
        return true;
    } else {
        Serial.println("\n[Wi-Fi] Wi-Fi DesConnected!");
        return false;
    }
}

WifiState WifiConnect::getState() const {
    return estadoAtual;
}

// ==========================================
// MÉTODOS ESTÁTICOS DE CONSULTA (Para a UI)
// ==========================================
const char* WifiConnect::obtainIP() {
    static char ip_str[16];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(ip_str, sizeof(ip_str), "0.0.0.0");
    }
    return ip_str;
}

const char* WifiConnect::obtainMascara() {
    static char mask_str[16];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(mask_str, sizeof(mask_str), "%s", WiFi.subnetMask().toString().c_str());
    } else {
        snprintf(mask_str, sizeof(mask_str), "0.0.0.0");
    }
    return mask_str;
}

const char* WifiConnect::obtainMacAddress() {
    static char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%s", WiFi.macAddress().c_str());
    return mac_str;
}