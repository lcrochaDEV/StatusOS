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

#include "physicalAccessControl.h"
PhysicalAccessControl physicalAccessControl; 

// Estrutura de Telemetria mantendo DADOS BRUTOS (raw data)
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
    
    // Dados Brutos em Bytes
    uint64_t ram_total_bytes = 0;
    uint64_t ram_used_bytes  = 0;
    uint64_t disk_total_bytes = 0;
    uint64_t disk_used_bytes  = 0;
    
    bool online = false;
    volatile bool updated = false;
    char datetime[64] = "";
    uint64_t epoch_timestamp = 0;
    uint32_t last_seen_ms = 0;
};

inline TelemetryData g_telemetry;
inline std::vector<TelemetryData> g_telemetry_list;

// Controle do carrossel do display
inline size_t g_current_display_index = 0;
inline uint32_t g_last_host_switch_ms = 0;
const uint32_t DISPLAY_ROTATION_INTERVAL_MS = 90000;

inline AsyncWebServer server(80);
inline AsyncWebSocket ws("/ws");

inline bool g_webserver_state = true;
inline String g_wake_up_time = "07:00";
inline String g_sleep_time = "22:00";

String processor(const String& var);

String processor(const String& var) {
    if(var == "SSID_VALUE") return (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "Desconectado";
    if(var == "IP_VALUE")   return (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "0.0.0.0";
    if(var == "MAC_VALUE")  return WiFi.macAddress();

    if(var == "MODULE_VALUE")         return physicalAccessControl.modelBoardESP();
    if (var == "TOTAL_RAM_VALUE")      return String((uint32_t)physicalAccessControl.total_ram_kb()) + " KB";
    if (var == "FLASH_SIZE_VALUE")     return String((uint32_t)physicalAccessControl.flash_size_mb()) + " MB";
    if (var == "MENOR_RAM_SIZE_VALUE") return String((uint32_t)physicalAccessControl.min_free_ram_kb()) + " KB";
    if (var == "SKETCH_SIZE_VALUE")    return String((uint32_t)physicalAccessControl.sketch_size_kb()) + " KB";
    if(var == "WEBSERVER_STATE") return g_webserver_state ? "checked" : "";
    if(var == "WAKE_TIME_VALUE") return g_wake_up_time;
    if(var == "SLEEP_TIME_VALUE") return g_sleep_time;
    
    return String("");
}

void handleDateTime(AsyncWebServerRequest *request) {
    // Obter o tempo atual do sistema (assumindo que o NTP foi configurado)
    time_t now = time(nullptr); 
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Alocação de memória para strings formatadas
    char dateString[11]; // DD/MM/YYYY + '\0'
    char hourString[9]; // HH:MM:SS + '\0'

    // Formata a data e hora
    strftime(dateString, sizeof(dateString), "%d/%m/%Y", &timeinfo);
    strftime(hourString, sizeof(hourString), "%H:%M:%S", &timeinfo);
    
    // Constrói a resposta JSON para o JavaScript
    String responseJson = "{\"date\":\"";
    responseJson += dateString;
    responseJson += "\",\"time\":\"";
    responseJson += hourString;
    responseJson += "\"}";

    // Envia a resposta JSON. É ESSENCIAL usar send() aqui.
    request->send(200, "application/json", responseJson);
}

inline TelemetryData* get_or_create_host(const char* id_or_name) {
    for (auto& item : g_telemetry_list) {
        if (strcmp(item.id, id_or_name) == 0 || strcmp(item.host, id_or_name) == 0) {
            return &item;
        }
    }
    TelemetryData new_host;
    strncpy(new_host.id, id_or_name, sizeof(new_host.id) - 1);
    strncpy(new_host.host, id_or_name, sizeof(new_host.host) - 1);
    g_telemetry_list.push_back(new_host);
    return &g_telemetry_list.back();
}

// Gera o JSON com os DADOS BRUTOS de RAM e Disco para o Front-End converter
inline String build_json_string() {
    JsonDocument doc;
    JsonArray hostsArray = doc.to<JsonArray>();

    for (const auto& item : g_telemetry_list) {
        JsonObject obj = hostsArray.add<JsonObject>();
        obj["id"] = item.id;
        obj["ip"] = item.ip;
        obj["mac"] = item.mac;
        obj["host"] = item.host;
        obj["datetime"] = item.datetime;
        obj["epoch_timestamp"] = item.epoch_timestamp;
        obj["uptime"] = item.uptime;
        obj["os_name"] = item.os_name;
        obj["kernel"] = item.kernel;
        obj["temp"] = item.temp;
        obj["cpu_load"] = item.cpu_load;
        
        // Envio de Dados Brutos em Bytes
        obj["ram_total_bytes"]  = item.ram_total_bytes;
        obj["ram_used_bytes"]   = item.ram_used_bytes;
        obj["disk_total_bytes"] = item.disk_total_bytes;
        obj["disk_used_bytes"]  = item.disk_used_bytes;
        
        obj["logo_url"] = item.logo_url;
        obj["online"]   = item.online;
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

inline void update_display_and_ws() {
    // Para o Display local TFT, se necessário, calculamos localmente
    int ram_pct = (g_telemetry.ram_total_bytes > 0) ? (int)((g_telemetry.ram_used_bytes * 100ULL) / g_telemetry.ram_total_bytes) : 0;
    int disk_pct = (g_telemetry.disk_total_bytes > 0) ? (int)((g_telemetry.disk_used_bytes * 100ULL) / g_telemetry.disk_total_bytes) : 0;

    if (ws.count() > 0) ws.textAll(build_json_string());


    update_telemetry_data(
        g_telemetry.host,
        g_telemetry.os_name,
        g_telemetry.kernel,
        g_telemetry.temp,
        g_telemetry.cpu_load,
        ram_pct,
        disk_pct,
        g_telemetry.uptime,
        g_telemetry.logo_url
    );
    update_status(g_telemetry.online);
}

// Extrator flexível de dados brutos do JSON recebido
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

    if (doc["servidor"].is<const char*>()) strncpy(target_host->host, doc["servidor"], sizeof(target_host->host) - 1);
    else if (doc["host"].is<const char*>()) strncpy(target_host->host, doc["host"], sizeof(target_host->host) - 1);

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

    // Leitura Flexível de Métricas de Memória e Disco (Bytes ou conversão automática de KB/MB caso o agente seja antigo)
    if (doc["metricas"].is<JsonObject>()) {
        JsonObject metricas = doc["metricas"];
        if (!metricas["cpu_temp"].isNull()) target_host->temp = metricas["cpu_temp"].as<float>();
        if (!metricas["cpu_load_1m"].isNull()) target_host->cpu_load = metricas["cpu_load_1m"].as<float>();

        if (metricas["memoria"].is<JsonObject>()) {
            JsonObject mem = metricas["memoria"];
            if (!mem["total_bytes"].isNull()) target_host->ram_total_bytes = mem["total_bytes"].as<uint64_t>();
            else if (!mem["total_kb"].isNull()) target_host->ram_total_bytes = mem["total_kb"].as<uint64_t>() * 1024ULL;
            else if (!mem["total_mb"].isNull()) target_host->ram_total_bytes = mem["total_mb"].as<uint64_t>() * 1024ULL * 1024ULL;

            if (!mem["usada_bytes"].isNull()) target_host->ram_used_bytes = mem["usada_bytes"].as<uint64_t>();
            else if (!mem["usada_kb"].isNull()) target_host->ram_used_bytes = mem["usada_kb"].as<uint64_t>() * 1024ULL;
            else if (!mem["usada_mb"].isNull()) target_host->ram_used_bytes = mem["usada_mb"].as<uint64_t>() * 1024ULL * 1024ULL;
        }

        if (metricas["disco"].is<JsonObject>()) {
            JsonObject dsk = metricas["disco"];
            if (!dsk["total_bytes"].isNull()) target_host->disk_total_bytes = dsk["total_bytes"].as<uint64_t>();
            else if (!dsk["total_kb"].isNull()) target_host->disk_total_bytes = dsk["total_kb"].as<uint64_t>() * 1024ULL;
            else if (!dsk["total_mb"].isNull()) target_host->disk_total_bytes = dsk["total_mb"].as<uint64_t>() * 1024ULL * 1024ULL;

            if (!dsk["usado_bytes"].isNull()) target_host->disk_used_bytes = dsk["usado_bytes"].as<uint64_t>();
            else if (!dsk["usado_kb"].isNull()) target_host->disk_used_bytes = dsk["usado_kb"].as<uint64_t>() * 1024ULL;
            else if (!dsk["usado_mb"].isNull()) target_host->disk_used_bytes = dsk["usado_mb"].as<uint64_t>() * 1024ULL * 1024ULL;
        }
    } else {
        if (!doc["temp"].isNull()) target_host->temp = doc["temp"].as<float>();
        if (!doc["cpu_load"].isNull()) target_host->cpu_load = doc["cpu_load"].as<float>();

        if (!doc["ram_total_bytes"].isNull()) target_host->ram_total_bytes = doc["ram_total_bytes"].as<uint64_t>();
        else if (!doc["ram_total_kb"].isNull()) target_host->ram_total_bytes = doc["ram_total_kb"].as<uint64_t>() * 1024ULL;
        
        if (!doc["ram_used_bytes"].isNull()) target_host->ram_used_bytes = doc["ram_used_bytes"].as<uint64_t>();
        else if (!doc["ram_used_kb"].isNull()) target_host->ram_used_bytes = doc["ram_used_kb"].as<uint64_t>() * 1024ULL;

        if (!doc["disk_total_bytes"].isNull()) target_host->disk_total_bytes = doc["disk_total_bytes"].as<uint64_t>();
        else if (!doc["disk_total_kb"].isNull()) target_host->disk_total_bytes = doc["disk_total_kb"].as<uint64_t>() * 1024ULL;

        if (!doc["disk_used_bytes"].isNull()) target_host->disk_used_bytes = doc["disk_used_bytes"].as<uint64_t>();
        else if (!doc["disk_used_kb"].isNull()) target_host->disk_used_bytes = doc["disk_used_kb"].as<uint64_t>() * 1024ULL;
    }

    target_host->online = true;
    target_host->updated = true;

    if (g_telemetry_list.size() == 1) {
        g_telemetry = *target_host;
        g_telemetry.updated = true;
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

inline void handle_post_telemetry(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
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
                ws.closeAll();
            }
        }
    }

    if (doc["wake_time"].is<const char*>()) g_wake_up_time = doc["wake_time"].as<String>();
    if (doc["sleep_time"].is<const char*>()) g_sleep_time = doc["sleep_time"].as<String>();

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
        if (!g_webserver_state) {
            request->send(503, "text/html", 
                "<html><head><meta charset='UTF-8'></head><body style='background:#0a0413;color:#fff;font-family:sans-serif;text-align:center;padding-top:100px;'>"
                "<h1>Serviço em Pausa</h1>"
                "<p>A telemetria do dashboard está temporariamente desativada.</p>"
                "<a href='/config' style='color:#00f2fe;'>Acessar Painel de Configurações</a>"
                "</body></html>"
            );
            return;
        }
        request->send(200, "text/html", index_html);
    });
      // NOVO: Rota para Data e Hora Dinâmicas (JSON)
    server.on("/datetime", HTTP_GET, handleDateTime); 

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", config_html, processor);
    });

    server.on("/autodiscovery", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });
    server.on("/autodiscovery", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!g_webserver_state) {
            request->send(503, "application/json", "{\"status\":\"paused\",\"message\":\"Serviço suspenso\"}");
            return;
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/telemetry", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });
    server.on("/api/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });

    server.on("/api/telemetry", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handle_post_telemetry);
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handle_post_config);

    server.begin();
    Serial.println("Servidor HTTP Async Iniciado!");
}

inline void process_web_server_tasks() {
    if (!g_webserver_state) return;

    ws.cleanupClients();
    uint32_t now = millis();

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