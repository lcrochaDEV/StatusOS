#include "WifiConnect.h"

// Construtor
WifiConnect::WifiConnect(const char* ssid, const char* password)
  : ssid(ssid), password(password) {
    // Registra os callbacks nativos da ESP-IDF
    registrarEventos();
}

// -------------------------------------------------------------------------
// PASSO 1: Configuração Inicial do Rádio (Assíncrona)
// -------------------------------------------------------------------------
void WifiConnect::connections_Wifi() {
    // 1. Se já estiver conectado, não faz nada
    if (WiFi.status() == WL_CONNECTED || _estado == REDE_CONECTADA) return;

    // Reset físico rápido para estabilização do chip
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);      // Preserva a vida útil da memória Flash
    WiFi.setAutoReconnect(true); // Reconexão automática para micro-quedas

    // Dispara a tentativa inicial sem laço bloqueante (while)
    _estado = REDE_CONECTANDO;
    _ultimaTentativaMs = millis();
    WiFi.begin(ssid, password);
}

// -------------------------------------------------------------------------
// PASSO 2: Manipulação Unificada de Eventos do Sistema
// -------------------------------------------------------------------------
void WifiConnect::registrarEventos() {
    // Evento: Conexão Estabelecida e IP Atribuído
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        _estado = REDE_CONECTADA;
        _intervaloReconexao = 2000; // Reseta o tempo de espera do Backoff Exponencial

        // Reinicia contador e exibe mensagem conforme a rede conectada
        if (ProcessoDeBackup) {
            Serial.println("\n[OK] Conectado via Backup!");
            ProcessoDeBackup = false; 
        } else {
            Serial.println("\n\n[OK] Conectado à rede principal!");
            Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
            Serial.printf("Endereco IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Endereco MAC do Gateway %s\n", WiFi.macAddress().c_str()); // Anote este MAC para usar no codigo do Sender
            Serial.printf("Canal Wi-Fi atual: %d\n\n", WiFi.channel());

            // Guarda os dados da rede principal como backup em cache
            backupSsid = WiFi.SSID();
            backupPass = WiFi.psk();
        }

    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    // Evento: Desconexão da Rede
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        // 🎯 PROTEÇÃO: Se o evento de desconexão chegou atrasado mas o Wi-Fi JÁ está conectado, ignora!
        if (WiFi.status() == WL_CONNECTED || _estado == REDE_CONECTADA) return;

        uint8_t motivo = info.wifi_sta_disconnected.reason;
        _estado = REDE_DESCONECTADA;

        Serial.printf("\n[Wi-Fi] ⚠️ Conexão perdida (Motivo SDK: %d).\n", motivo);

    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

// -------------------------------------------------------------------------
// PASSO 3: Máquina de Estados e Reconexão com Fallback (Loop Principal)
// -------------------------------------------------------------------------
void WifiConnect::loop() {
    if (_estado == REDE_DESCONECTADA) {
        unsigned long agora = millis();

        if (agora - _ultimaTentativaMs >= _intervaloReconexao) {
            _ultimaTentativaMs = agora;
            _estado = REDE_CONECTANDO;

            // Se o tempo de espera acumulado atingiu o limite intermediário (ex: 16s) e 
            // houver credenciais salvas de backup, chaveia automaticamente o SSID
            if (_intervaloReconexao >= 16000 && backupSsid.length() > 0 && !ProcessoDeBackup) {
                ProcessoDeBackup = true;
                Serial.printf("[Wi-Fi] 🔄 Alternando para rede de backup: %s\n", backupSsid.c_str());
                WiFi.disconnect(false, false);
                WiFi.begin(backupSsid.c_str(), backupPass.c_str());
            } else {
                const char* ssidAlvo = ProcessoDeBackup ? backupSsid.c_str() : ssid;
                const char* passAlvo = ProcessoDeBackup ? backupPass.c_str() : password;

                if (ProcessoDeBackup) {
                    Serial.printf("[Wi-Fi] 📡 Tentando reconectar via BACKUP (%s) - Próxima em %lus...\n", 
                                  ssidAlvo, _intervaloReconexao / 1000);
                } else {
                    Serial.printf("[Wi-Fi] 📡 Tentando reconectar na PRINCIPAL (%s) - Próxima em %lus...\n", 
                                  ssidAlvo, _intervaloReconexao / 1000);
                }

                WiFi.disconnect(false, false);
                WiFi.begin(ssidAlvo, passAlvo);
            }

            // Expande gradualmente o intervalo até o teto de 60s
            _intervaloReconexao = min(_intervaloReconexao * 2, _intervaloMaximo);
        }
    }
}

// -------------------------------------------------------------------------
// PASSO 4: Métodos Utilitários e Diagnósticos
// -------------------------------------------------------------------------
bool WifiConnect::connections_status() {
    return (WiFi.status() == WL_CONNECTED);
}

void WifiConnect::diconnectRede() {
    WiFi.disconnect(true);
    _estado = REDE_DESCONECTADA;
    Serial.println("[Wi-Fi] Desconectando da rede...");
}

void WifiConnect::wifiOff() {
    diconnectRede();
    WiFi.mode(WIFI_OFF);
    Serial.println("[Wi-Fi] Rádio Wi-Fi desligado.");
}

void WifiConnect::searchRedes() {
    int n = WiFi.scanNetworks();
    Serial.printf("\n[Wi-Fi] Redes encontradas: %d\n", n);

    if (n == 0) {
        Serial.println("[Wi-Fi] Nenhuma rede encontrada.");
    } else {
        for (int i = 0; i < n; ++i) {
            Serial.printf(" %d: %s (%d dBm) - Segurança: ", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            uint8_t encryptionType = WiFi.encryptionType(i);
            Serial.println(getEncryptionName(encryptionType));
        }
    }
    WiFi.scanDelete();
}

String WifiConnect::getEncryptionName(uint8_t encryptionType) {
    #if defined(ESP32)
        switch (encryptionType) {
            case WIFI_AUTH_OPEN:             return "Aberta";
            case WIFI_AUTH_WEP:              return "WEP";
            case WIFI_AUTH_WPA_PSK:          return "WPA-PSK";
            case WIFI_AUTH_WPA2_PSK:         return "WPA2-PSK";
            case WIFI_AUTH_WPA_WPA2_PSK:     return "WPA/WPA2-PSK";
            case WIFI_AUTH_WPA2_ENTERPRISE:  return "WPA2-Enterprise";
            case WIFI_AUTH_WPA3_PSK:         return "WPA3-SAE";
            case WIFI_AUTH_WPA2_WPA3_PSK:    return "WPA2/WPA3 Transition";
            default:                         return "Protegida";
        }
    #elif defined(ESP8266)
        switch (encryptionType) {
            case ENC_TYPE_NONE: return "Aberta";
            case ENC_TYPE_WEP:  return "WEP";
            case ENC_TYPE_TKIP: return "WPA (TKIP)";
            case ENC_TYPE_CCMP: return "WPA2 (AES)";
            case ENC_TYPE_AUTO: return "WPA/WPA2 Auto";
            default:            return "Protegida";
        }
    #endif
    return "Protegida";
}