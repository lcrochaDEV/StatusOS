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

WifiConnect::WifiConnect(const char* ssid, const char* password)
  : ssid(ssid), password(password) {
    if (ssid && strlen(ssid) > 0) {
        snprintf(ssidBuffer, sizeof(ssidBuffer), "%s", ssid);
        if (password) snprintf(passwordBuffer, sizeof(passwordBuffer), "%s", password);
        this->ssid = ssidBuffer;
        this->password = passwordBuffer;
    }
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
    Serial.println("[Wi-Fi] Credenciais apagadas. Abrindo Access Point...");

    Preferences prefs;
    if (prefs.begin(NVS_WIFI_NAMESPACE, false)) {
        prefs.remove(NVS_WIFI_KEY_SSID);
        prefs.remove(NVS_WIFI_KEY_PASS);
        prefs.end();
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
        connectManual();
    } else if (possuiCredenciais()) {
        // Só são gravadas na memória após a primeira conexão bem-sucedida (ver loop)
        connectManual();
    } else {
        startAccessPoint();
    }
}

// Inicializa a tentativa de conexão manual de forma ASSÍNCRONA
void WifiConnect::connectManual() {
    if (ssid == nullptr || strlen(ssid) == 0) {
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
        startAccessPoint();
    }
}

// Processamento Não Bloqueante executado continuamente no loop()
void WifiConnect::loop() {
    // Portal ativo: processa o WiFiManager de forma não bloqueante (display e web continuam rodando)
    if (portalAtivo) {
        wm.process();

        if (WiFi.status() == WL_CONNECTED) {
            // Rede configurada no portal: grava as credenciais e reinicia uma única vez. As conexões HTTP do
            // portal deixam a porta 80 ocupada no lwIP (TIME_WAIT), o que impediria o servidor web de abrir.
            snprintf(ssidBuffer, sizeof(ssidBuffer), "%s", WiFi.SSID().c_str());
            snprintf(passwordBuffer, sizeof(passwordBuffer), "%s", WiFi.psk().c_str());
            ssid = ssidBuffer;
            password = passwordBuffer;
            salvarCredenciais();

            Serial.println("[Wi-Fi] Rede configurada pelo Portal AP. Reiniciando...");
            ESP.restart();
        }
        else if (!wm.getConfigPortalActive()) {
            Serial.println("[Wi-Fi] Tempo limite do Portal atingido.");
            portalAtivo = false;
            estadoAtual = WifiState::Failure;
            ultimaTentativaReconexao = millis();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
        }
        return;
    }

    // 1. Monitora o processo de Conexão Ativa
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
    // 2. Reconexão Automática Passiva (Sem travar o processador)
    else if (estadoAtual == WifiState::Failure || (estadoAtual == WifiState::Connected && WiFi.status() != WL_CONNECTED)) {
        if (millis() - ultimaTentativaReconexao >= intervaloReconexaoMs) {
            connectManual();
        }
    }
}

// Portal de Configuração com liberação de recursos
void WifiConnect::startAccessPoint(const char* apPassword, uint32_t timeoutSegundos) {
    if (portalAtivo) return; // Portal já aberto

    falhasCredencial = 0;

    WiFi.disconnect();

    // Configura o portal de forma profissional
    wm.setDebugOutput(false);
    wm.setConfigPortalBlocking(false); // Não trava o loop(): LVGL e servidor seguem rodando
    wm.setConfigPortalTimeout(timeoutSegundos); // Não fica travado para sempre se ninguém conectar

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

// ==========================================
// MÉTODOS ESTÁTICOS DE CONSULTA (Para a UI)
// ==========================================
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