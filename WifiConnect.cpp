#include "WifiConnect.h"
#include <Preferences.h>
#include <WiFiManager.h>

#if defined(ESP32)
  #include <esp_wifi.h>
#endif

static const char* NVS_WIFI_NAMESPACE = "wificonn";
static const char* NVS_WIFI_KEY_SSID = "ssid";
static const char* NVS_WIFI_KEY_PASS = "pass";

// Portal único e não bloqueante (processado em loop()), sem alocação na stack
static WiFiManager wm;
static bool portalAtivo = false;
static volatile uint8_t motivoDesconexao = 0; // Último motivo de queda do STA (evento do Wi-Fi)

// Ciclo do rádio após o Portal AP (substitui reboot): desliga o Wi-Fi, aguarda e religa em STA limpo
static bool reiniciandoRadio = false;
static uint32_t tempoRadioOff = 0;
static const uint32_t intervaloRadioOffMs = 300;

WifiConnect::WifiConnect(const char* ssid, const char* password)
  : ssid(ssid), password(password) {
    if (ssid && strlen(ssid) > 0) {
        snprintf(ssidBuffer, sizeof(ssidBuffer), "%s", ssid);
        if (password) snprintf(passwordBuffer, sizeof(passwordBuffer), "%s", password);
        this->ssid = ssidBuffer;
        this->password = passwordBuffer;
    }
}

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

bool WifiConnect::possuiCredenciais() const {
    return ssid != nullptr && strlen(ssid) > 0;
}

// Única declaração de carregarCredenciais (sem duplicidade)
bool WifiConnect::carregarCredenciais() {
    Preferences prefs;
    if (!prefs.begin(NVS_WIFI_NAMESPACE, true)) return false;

    char ssidSalvo[33] = {0};
    char senhaSalva[65] = {0};
    prefs.getString(NVS_WIFI_KEY_SSID, ssidSalvo, sizeof(ssidSalvo));
    prefs.getString(NVS_WIFI_KEY_PASS, senhaSalva, sizeof(senhaSalva));
    prefs.end();

    if (strlen(ssidSalvo) == 0) return false;

    snprintf(ssidBuffer, sizeof(ssidBuffer), "%s", ssidSalvo);
    snprintf(passwordBuffer, sizeof(passwordBuffer), "%s", senhaSalva);
    ssid = ssidBuffer;
    password = passwordBuffer;
    return true;
}

void WifiConnect::salvarCredenciais() {
    if (!possuiCredenciais()) return;
    Preferences prefs;
    if (!prefs.begin(NVS_WIFI_NAMESPACE, false)) return;
    prefs.putString(NVS_WIFI_KEY_SSID, ssid);
    prefs.putString(NVS_WIFI_KEY_PASS, password ? password : "");
    prefs.end();
}

void WifiConnect::clearCredentials() {
    Serial.println("\n[Wi-Fi] Solicitação de limpeza de credenciais recebida!");

    Preferences prefs;
    if (prefs.begin(NVS_WIFI_NAMESPACE, false)) {
        prefs.remove(NVS_WIFI_KEY_SSID);
        prefs.remove(NVS_WIFI_KEY_PASS);
        prefs.end();
        Serial.println("[Wi-Fi] Credenciais apagadas da memória Flash com sucesso.");
    }

    ssidBuffer[0] = '\0';
    passwordBuffer[0] = '\0';
    ssid = ssidBuffer;
    password = passwordBuffer;
    credenciaisConfirmadas = false;
    falhasCredencial = 0;

    WiFi.disconnect();
    startAccessPoint();
}

void WifiConnect::begin() {
#if defined(ESP32)
    // Registra uma única vez a captura do motivo da queda (diferencia credencial rejeitada de rede offline)
    static bool eventoRegistrado = false;
    if (!eventoRegistrado) {
        WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
            motivoDesconexao = info.wifi_sta_disconnected.reason;
        }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        eventoRegistrado = true;
    }
#endif

    if (carregarCredenciais()) {
        credenciaisConfirmadas = true;
        Serial.println("[Wi-Fi] Credenciais carregadas da memória. Conectando manualmente...");
        connectManual();
    } else if (possuiCredenciais()) {
        // Só são gravadas na memória após a primeira conexão bem-sucedida (ver loop)
        connectManual();
    } else {
        Serial.println("[Wi-Fi] Nenhuma credencial encontrada. Abrindo Access Point...");
        startAccessPoint();
    }
}

void WifiConnect::connectManual() {
    if (ssid == nullptr || strlen(ssid) == 0) {
        Serial.println("[Wi-Fi] Erro: SSID não fornecido.");
        startAccessPoint();
        return;
    }

    Serial.printf("[Wi-Fi] Iniciando conexão assíncrona a: %s\n", ssid);
    
    motivoDesconexao = 0;
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    tempoInicioConexao = millis();
    estadoAtual = WifiState::Connecting;
}

void WifiConnect::avaliarFalhaDeConexao() {
    // Nunca conectou: qualquer falha abre o Access Point para configurar a rede
    if (!credenciaisConfirmadas) {
        Serial.println("[Wi-Fi] Falha na primeira conexão. Abrindo Access Point...");
        startAccessPoint();
        return;
    }

    // Rede offline não abre o A.P.; somente credencial rejeitada repetidamente (credenciais mudaram)
    uint8_t motivo = motivoDesconexao;
    bool credencialRejeitada = (motivo == WIFI_REASON_AUTH_FAIL || motivo == WIFI_REASON_ASSOC_FAIL ||
                                motivo == WIFI_REASON_HANDSHAKE_TIMEOUT || motivo == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT);
    if (!credencialRejeitada) {
        falhasCredencial = 0;
        return;
    }

    falhasCredencial++;
    if (falhasCredencial >= limiteFalhasCredencial) {
        Serial.println("[Wi-Fi] Credenciais rejeitadas repetidamente. Abrindo Access Point...");
        startAccessPoint();
    }
}

void WifiConnect::loop() {
    // Aguarda o rádio desligado (não bloqueante) e retoma pelo fluxo normal de conexão, igual ao boot
    if (reiniciandoRadio) {
        if (millis() - tempoRadioOff >= intervaloRadioOffMs) {
            reiniciandoRadio = false;
            connectManual();
        }
        return;
    }

    // Portal ativo: processa o WiFiManager de forma não bloqueante (display e web continuam rodando)
    if (portalAtivo) {
        wm.process();

        // Verifica se o WiFiManager terminou com sucesso (conectado à rede alvo)
        // Usamos wm.getConfigPortalActive() ou verificamos se o IP foi atribuído no modo STA
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[Wi-Fi] Configuração via Portal AP concluída com sucesso!");
            
            // 1. Salva imediatamente as credenciais validadas na NVS antes de mexer no rádio
            snprintf(ssidBuffer, sizeof(ssidBuffer), "%s", WiFi.SSID().c_str());
            snprintf(passwordBuffer, sizeof(passwordBuffer), "%s", WiFi.psk().c_str());
            ssid = ssidBuffer;
            password = passwordBuffer;
            salvarCredenciais();
            credenciaisConfirmadas = true;
            falhasCredencial = 0;

            // 2. Encerra o portal e o A.P., e cicla o rádio para sair do modo AP+STA com a pilha limpa
            portalAtivo = false;

            // Libera a porta 80 do portal para o servidor web (só se o WiFiManager ainda não o encerrou sozinho)
            if (wm.getConfigPortalActive()) wm.stopConfigPortal();

            WiFi.softAPdisconnect(true);
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);

            estadoAtual = WifiState::Disconnected;
            tempoRadioOff = millis();
            reiniciandoRadio = true;
        } 
        else if (!wm.getConfigPortalActive()) {
            Serial.println("[Wi-Fi] Tempo limite do Portal atingido. Retomando conexão manual...");
            portalAtivo = false;
            estadoAtual = WifiState::Failure;
            ultimaTentativaReconexao = millis();
            falhasCredencial = 0;
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
        }
        return;
    }

    if (estadoAtual == WifiState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            estadoAtual = WifiState::Connected;
            falhasCredencial = 0;
            if (!credenciaisConfirmadas) {
                credenciaisConfirmadas = true;
                salvarCredenciais();
            }
            checkStatus();
        } else if (millis() - tempoInicioConexao >= timeoutConexaoMs) {
            Serial.println("\n[Wi-Fi] Tempo limite de conexão esgotado!");
            estadoAtual = WifiState::Failure;
            ultimaTentativaReconexao = millis();
            avaliarFalhaDeConexao();
        }
    } 
    else if (estadoAtual == WifiState::Failure || (estadoAtual == WifiState::Connected && WiFi.status() != WL_CONNECTED)) {
        if (millis() - ultimaTentativaReconexao >= intervaloReconexaoMs) {
            Serial.println("[Wi-Fi] Tentando reconectar à rede...");
            connectManual();
        }
    }
}

void WifiConnect::startAccessPoint(const char* apPassword, uint32_t timeoutSegundos) {
    if (portalAtivo) return; // Portal já aberto

    falhasCredencial = 0;

    WiFi.disconnect();

    wm.setDebugOutput(false);
    wm.setConfigPortalBlocking(false); // Não trava o loop(): LVGL e servidor seguem rodando
    wm.setConfigPortalTimeout(timeoutSegundos);

    char macStr[13];
    extrairMacHardware(macStr, sizeof(macStr));
    
    char ssidFinal[32];
    snprintf(ssidFinal, sizeof(ssidFinal), "ESP_%s", macStr);

    Serial.printf("[Wi-Fi] Abrindo Portal AP: %s\n", ssidFinal);

    WiFi.persistent(false);

    if (apPassword && strlen(apPassword) > 0) {
        wm.startConfigPortal(ssidFinal, apPassword);
    } else {
        wm.startConfigPortal(ssidFinal);
    }

#if defined(ESP32)
    // Rádio sem economia de energia estabiliza a conexão dos dispositivos ao A.P.
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif

    portalAtivo = true;
    estadoAtual = WifiState::Disconnected;
}

bool WifiConnect::checkStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[Wi-Fi] Conectado à Rede: %s\n", WiFi.SSID().c_str()); 
        Serial.printf("[Wi-Fi] Endereço IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[Wi-Fi] Endereço MAC: %s\n", WiFi.macAddress().c_str()); 
        Serial.printf("[Wi-Fi] Canal Wi-Fi: %d\n\n", WiFi.channel()); 
        return true;
    } else {
        Serial.println("\n[Wi-Fi] Wi-Fi Desconectado!");
        return false;
    }
}

WifiState WifiConnect::getState() const {
    return estadoAtual;
}

const char* WifiConnect::obtainIP() {
    static char ip_str[16];
    if (portalAtivo) snprintf(ip_str, sizeof(ip_str), "%s", WiFi.softAPIP().toString().c_str());
    else if (WiFi.status() == WL_CONNECTED) snprintf(ip_str, sizeof(ip_str), "%s", WiFi.localIP().toString().c_str());
    else snprintf(ip_str, sizeof(ip_str), "0.0.0.0");
    return ip_str;
}

const char* WifiConnect::obtainMascara() {
    static char mask_str[16];
    if (portalAtivo) snprintf(mask_str, sizeof(mask_str), "255.255.255.0");
    else if (WiFi.status() == WL_CONNECTED) snprintf(mask_str, sizeof(mask_str), "%s", WiFi.subnetMask().toString().c_str());
    else snprintf(mask_str, sizeof(mask_str), "0.0.0.0");
    return mask_str;
}

const char* WifiConnect::obtainMacAddress() {
    static char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%s", WiFi.macAddress().c_str());
    return mac_str;
}