#ifndef SERVIDORWEB_H
#define SERVIDORWEB_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <vector>
#include "ui_dashboard.h"
#include "index_html.h"
#include "config_html.h"

#include "PhysicalAccessControl.h"
PhysicalAccessControl physicalAccessControl; 

struct TelemetryData {
    char id[64] = "N/A";
    char ip[64] = "0.0.0.0";
    char mac[64] = "00:00:00:00:00:00";
    char host[128] = "Desconhecido";
    char os_name[128] = "Linux/Windows";
    char kernel[128] = "N/A";
    char uptime[64] = "--:--";
    float temp = 0.0f;
    char logo_url[256] = "";
    float cpu_load = 0.0f;
    int ram_pct = 0;
    int ram_total_mb = 0;
    int ram_used_mb = 0;
    int disk_pct = 0;
    float disk_total_gb = 0.0f;
    float disk_used_gb = 0.0f;
    bool online = false;
    volatile bool updated = false;
    char datetime[64] = "";
    uint64_t epoch_timestamp = 0;
    uint32_t last_seen_ms = 0;
};

inline TelemetryData g_telemetry;
inline std::vector<TelemetryData> g_telemetry_list;

// Controle de carrossel do display TFT (1min 30s)
inline size_t g_current_display_index = 0;
inline uint32_t g_last_host_switch_ms = 0;
const uint32_t DISPLAY_ROTATION_INTERVAL_MS = 90000;

inline AsyncWebServer server(80);
inline AsyncWebSocket ws("/ws");

// Variáveis Globais de Estado do Servidor/Configurações
inline bool g_webserver_state = true;
inline String g_wake_up_time = "07:00";
inline String g_sleep_time = "22:00";

// Protótipos de Funções
String processor(const String& var);

// Processador de Placeholders do HTML
String processor(const String& var) {
    if(var == "SSID_VALUE") return (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "Desconectado";
    if(var == "IP_VALUE")   return (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "0.0.0.0";
    if(var == "MAC_VALUE")  return WiFi.macAddress();

    if(var == "MODULE_VALUE")         return physicalAccessControl.modelBoardESP();
    if(var == "TOTAL_RAM_VALUE")      return physicalAccessControl.total_ram();
    if(var == "FLASH_SIZE_VALUE")     return physicalAccessControl.flash_size();
    if(var == "MENOR_RAM_SIZE_VALUE") return physicalAccessControl.menor_ram_size();
    if(var == "SKETCH_SIZE_VALUE")    return physicalAccessControl.sketch_Size();

    // Placeholders do Form/Switch
    if(var == "WEBSERVER_STATE") return g_webserver_state ? "checked" : "";
    if(var == "WAKE_TIME_VALUE") return g_wake_up_time;
    if(var == "SLEEP_TIME_VALUE") return g_sleep_time;
    
    return String("");
}

// Localiza ou cadastra host no vetor global por ID
inline TelemetryData* get_or_create_host(const char* host_id) {
    for (auto& item : g_telemetry_list) {
        if (strcmp(item.id, host_id) == 0) return &item;
    }
    TelemetryData new_host;
    strncpy(new_host.id, host_id, sizeof(new_host.id) - 1);
    new_host.last_seen_ms = millis();
    g_telemetry_list.push_back(new_host);
    return &g_telemetry_list.back();
}

inline String build_json_string() {
    JsonDocument doc;
    JsonArray hostsArray = doc.to<JsonArray>();

    for (const auto& item : g_telemetry_list) {
        JsonObject obj = hostsArray.add<JsonObject>();
        obj["id"] = item.id;
        obj["ip"] = item.ip;
        obj["mac"] = item.mac;
        obj["server"] = item.host;
        obj["host"] = item.host;
        obj["datetime"] = item.datetime;
        obj["epoch_timestamp"] = item.epoch_timestamp;
        obj["uptime"] = item.uptime;
        obj["os_name"] = item.os_name;
        obj["kernel"] = item.kernel;
        obj["temp"] = item.temp;
        obj["cpu_load"] = item.cpu_load;
        obj["ram_pct"] = item.ram_pct;
        obj["ram_total_mb"] = item.ram_total_mb;
        obj["ram_used_mb"] = item.ram_used_mb;
        obj["disk_pct"] = item.disk_pct;
        obj["disk_total_gb"] = item.disk_total_gb;
        obj["disk_used_gb"] = item.disk_used_gb;
        obj["logo_url"] = item.logo_url;
        obj["online"] = item.online;
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

inline void update_display_and_ws() {
    if (ws.count() > 0) ws.textAll(build_json_string());

    update_telemetry_data(
        g_telemetry.host,
        g_telemetry.os_name,
        g_telemetry.kernel,
        g_telemetry.temp,
        g_telemetry.cpu_load,
        g_telemetry.ram_pct,
        g_telemetry.disk_pct,
        g_telemetry.uptime,
        g_telemetry.logo_url
    );
    update_status(g_telemetry.online);
}

inline bool parse_telemetry_json(uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) return false;

    const char* host_id = "N/A";
    if (doc["id"].is<const char*>()) host_id = doc["id"];
    else if (doc["servidor"].is<const char*>()) host_id = doc["servidor"];
    else if (doc["host"].is<const char*>()) host_id = doc["host"];

    TelemetryData* target_host = get_or_create_host(host_id);
    target_host->last_seen_ms = millis();

    if (doc["id"].is<const char*>()) strncpy(target_host->id, doc["id"], sizeof(target_host->id) - 1);
    if (doc["ip"].is<const char*>()) strncpy(target_host->ip, doc["ip"], sizeof(target_host->ip) - 1);
    if (doc["mac"].is<const char*>()) strncpy(target_host->mac, doc["mac"], sizeof(target_host->mac) - 1);

    if (doc["servidor"].is<const char*>()) {
        strncpy(target_host->host, doc["servidor"], sizeof(target_host->host) - 1);
    } else if (doc["host"].is<const char*>()) {
        strncpy(target_host->host, doc["host"], sizeof(target_host->host) - 1);
    }

    if (doc["datetime"].is<const char*>()) strncpy(target_host->datetime, doc["datetime"], sizeof(target_host->datetime) - 1);
    if (!doc["epoch_timestamp"].isNull()) target_host->epoch_timestamp = doc["epoch_timestamp"].as<uint64_t>();
    if (doc["uptime"].is<const char*>()) strncpy(target_host->uptime, doc["uptime"], sizeof(target_host->uptime) - 1);
    if (doc["logo_url"].is<const char*>()) strncpy(target_host->logo_url, doc["logo_url"], sizeof(target_host->logo_url) - 1);

    if (doc["sistema_operacional"].is<JsonObject>()) {
        JsonObject so = doc["sistema_operacional"];
        if (so["nome"].is<const char*>()) strncpy(target_host->os_name, so["nome"], sizeof(target_host->os_name) - 1);
        if (so["kernel"].is<const char*>()) strncpy(target_host->kernel, so["kernel"], sizeof(target_host->kernel) - 1);
        if (so["logo_url"].is<const char*>()) strncpy(target_host->logo_url, so["logo_url"], sizeof(target_host->logo_url) - 1);
    } else {
        if (doc["os_name"].is<const char*>()) strncpy(target_host->os_name, doc["os_name"], sizeof(target_host->os_name) - 1);
        if (doc["kernel"].is<const char*>()) strncpy(target_host->kernel, doc["kernel"], sizeof(target_host->kernel) - 1);
    }

    if (doc["metricas"].is<JsonObject>()) {
        JsonObject metricas = doc["metricas"];
        if (!metricas["cpu_temp"].isNull()) target_host->temp = metricas["cpu_temp"].as<float>();
        if (!metricas["cpu_load_1m"].isNull()) target_host->cpu_load = metricas["cpu_load_1m"].as<float>();

        if (metricas["memoria"].is<JsonObject>()) {
            JsonObject memoria = metricas["memoria"];
            if (!memoria["percentual"].isNull()) target_host->ram_pct = memoria["percentual"].as<int>();
            if (!memoria["total_mb"].isNull()) target_host->ram_total_mb = memoria["total_mb"].as<int>();
            if (!memoria["usada_mb"].isNull()) target_host->ram_used_mb = memoria["usada_mb"].as<int>();
        }

        if (metricas["disco"].is<JsonObject>()) {
            JsonObject disco = metricas["disco"];
            if (!disco["uso_percentual"].isNull()) target_host->disk_pct = disco["uso_percentual"].as<int>();
            if (!disco["total_gb"].isNull()) target_host->disk_total_gb = disco["total_gb"].as<float>();
            if (!disco["usado_gb"].isNull()) target_host->disk_used_gb = disco["usado_gb"].as<float>();
        }
    } else {
        if (!doc["temp"].isNull()) target_host->temp = doc["temp"].as<float>();
        if (!doc["cpu_load"].isNull()) target_host->cpu_load = doc["cpu_load"].as<float>();
        if (!doc["ram_pct"].isNull()) target_host->ram_pct = doc["ram_pct"].as<int>();
        if (!doc["ram_total_mb"].isNull()) target_host->ram_total_mb = doc["ram_total_mb"].as<int>();
        if (!doc["ram_used_mb"].isNull()) target_host->ram_used_mb = doc["ram_used_mb"].as<int>();
        if (!doc["disk_pct"].isNull()) target_host->disk_pct = doc["disk_pct"].as<int>();
        if (!doc["disk_total_gb"].isNull()) target_host->disk_total_gb = doc["disk_total_gb"].as<float>();
        if (!doc["disk_used_gb"].isNull()) target_host->disk_used_gb = doc["disk_used_gb"].as<float>();
    }

    target_host->online = true;
    target_host->updated = true;

    if (g_telemetry_list.size() == 1) {
        g_telemetry = *target_host;
        g_telemetry.updated = true;
    } else {
        if (g_current_display_index < g_telemetry_list.size() &&
            strcmp(g_telemetry_list[g_current_display_index].id, target_host->id) == 0) {
            g_telemetry = *target_host;
            g_telemetry.updated = true;
        } else {
            if (ws.count() > 0) ws.textAll(build_json_string());
        }
    }

    return true;
}

inline void on_ws_event(AsyncWebSocket *server_ptr, AsyncWebSocketClient *client, 
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (!g_webserver_state) return;

    if (type == WS_EVT_CONNECT) {
        client->text(build_json_string());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            parse_telemetry_json(data, len);
        }
    }
}

// Handler para o POST /api/telemetry
inline void handle_post_telemetry(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Bloqueia e envia o 503 apenas no primeiro chunk (index == 0)
    if (!g_webserver_state) {
        if (index == 0) {
            request->send(503, "application/json", "{\"status\":\"paused\",\"message\":\"Serviço suspenso via painel\"}");
        }
        return;
    }

    if (index + len < total) return;

    if (!parse_telemetry_json(data, len)) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON Invalido\"}");
        return;
    }

    request->send(200, "application/json", "{\"status\":\"sucess\"}");
}

// Handler para salvar as configurações enviadas da página web (/config)
inline void handle_post_config(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len < total) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON Invalido\"}");
        return;
    }

    if (!doc["webserver"].isNull()) {
        bool newState = doc["webserver"].as<bool>();
        if (g_webserver_state != newState) {
            g_webserver_state = newState;
            if (!g_webserver_state) {
                ws.closeAll(); // Encerra WebSockets ativos se o serviço for pausado
            }
        }
    }

    if (doc["wake_time"].is<const char*>()) {
        g_wake_up_time = doc["wake_time"].as<String>();
    }

    if (doc["sleep_time"].is<const char*>()) {
        g_sleep_time = doc["sleep_time"].as<String>();
    }

    request->send(200, "application/json", "{\"status\":\"sucess\"}");
}

inline void setup_web_server() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

    ws.onEvent(on_ws_event);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Dashboard Server Online");
    });

    server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", config_html, processor);
    });

    // CORS Options Pre-flight
    server.on("/api/telemetry", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });
    server.on("/api/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });

    // API Rota Telemetria
    server.on("/api/telemetry", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        handle_post_telemetry
    );

    // API Rota Configurações
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        handle_post_config
    );

    server.begin();
    Serial.println("Servidor HTTP Async Iniciado!");
}

inline void process_web_server_tasks() {
    // Se o serviço estiver desligado pelo switch, congela os loops pesados
    if (!g_webserver_state) return;

    ws.cleanupClients();

    uint32_t now = millis();

    // Verificação de expiração de hosts
    if (!g_telemetry_list.empty()) {
        uint32_t timeout_threshold_ms = DISPLAY_ROTATION_INTERVAL_MS * 2 * g_telemetry_list.size();
        bool list_changed = false;

        for (auto it = g_telemetry_list.begin(); it != g_telemetry_list.end(); ) {
            if (now - it->last_seen_ms > timeout_threshold_ms) {
                it = g_telemetry_list.erase(it);
                list_changed = true;
            } else {
                ++it;
            }
        }

        if (list_changed) {
            if (g_telemetry_list.empty()) {
                g_current_display_index = 0;
                g_telemetry = TelemetryData();
                g_telemetry.updated = true;
            } else {
                if (g_current_display_index >= g_telemetry_list.size()) {
                    g_current_display_index = 0;
                }
                g_telemetry = g_telemetry_list[g_current_display_index];
                g_telemetry.updated = true;
            }
        }
    }

    // Rotação do carrossel do display
    if (!g_telemetry_list.empty()) {
        if (now - g_last_host_switch_ms >= DISPLAY_ROTATION_INTERVAL_MS) {
            g_last_host_switch_ms = now;
            g_current_display_index = (g_current_display_index + 1) % g_telemetry_list.size();
            g_telemetry = g_telemetry_list[g_current_display_index];
            g_telemetry.updated = true;
        }
    }

    if (g_telemetry.updated) {
        g_telemetry.updated = false;
        update_display_and_ws();
    }
}

#endif // SERVIDORWEB_H