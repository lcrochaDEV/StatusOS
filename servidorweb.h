#ifndef SERVIDORWEB_H
#define SERVIDORWEB_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mutex>
#include "SecurityConfig.h"
#include "ui_dashboard.h"
#include "index_html.h"
#include "config_html.h"
#include "TelemetryData.h"
#include "TelemetryParser.h"
#include "Hours_Time.h"

#include "PhysicalAccessControl.h"
inline PhysicalAccessControl physicalAccessControl;

inline AsyncWebServer server(80);
inline AsyncWebSocket ws("/ws");

inline volatile bool g_webserver_state = true;

// Alterações de configuração recebidas em /api/config são aplicadas na task principal (process_web_server_tasks)
inline volatile bool g_config_dirty = false;

constexpr const char* PREFS_NAMESPACE = "statusos";

// --------------------------------------------------
// Persistência (NVS)
// --------------------------------------------------
inline void load_config_from_nvs() {
    Preferences preferences;
    if (!preferences.begin(PREFS_NAMESPACE, true)) return;

    // O padrão dos horários é o valor atual do relógio (Hours_Time é a fonte única)
    String wake;
    String sleep;
    Hours_Time* clock = Hours_Time::instance();
    if (clock != nullptr) {
        clock->getHoursWakeon(wake);
        clock->getHoursSleep(sleep);
    }

    g_webserver_state = preferences.getBool("webserver", g_webserver_state);
    wake = preferences.getString("wake", wake);
    sleep = preferences.getString("sleep", sleep);
    preferences.end();

    // Os setters descartam horários inválidos
    if (clock != nullptr) {
        clock->setHoursWakeon(wake);
        clock->setHoursSleep(sleep);
    }
}

inline void save_config_to_nvs() {
    Preferences preferences;
    if (!preferences.begin(PREFS_NAMESPACE, false)) return;

    String wake;
    String sleep;
    Hours_Time* clock = Hours_Time::instance();
    if (clock != nullptr) {
        clock->getHoursWakeon(wake);
        clock->getHoursSleep(sleep);
    }

    preferences.putBool("webserver", g_webserver_state);
    if (clock != nullptr) {
        preferences.putString("wake", wake);
        preferences.putString("sleep", sleep);
    }
    preferences.end();
}

// --------------------------------------------------
// Utilitários HTTP
// --------------------------------------------------
// Escapa texto que será inserido no HTML (ex.: SSID controlado por terceiros)
inline String html_escape(const String& raw) {
    String out;
    out.reserve(raw.length() + 8);
    for (size_t i = 0; i < raw.length(); i++) {
        char c = raw[i];
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
    }
    return out;
}

// Token opcional (SecurityConfig.h): vazio = autenticação desativada
inline bool is_authorized(AsyncWebServerRequest *request) {
    if (strlen(API_TOKEN) == 0) return true;
    if (!request->hasHeader("X-Auth-Token")) return false;
    return request->header("X-Auth-Token").equals(API_TOKEN);
}

enum class BodyState { Incomplete, Complete, Rejected };

// Acumula o corpo da requisição (que pode chegar fragmentado) em request->_tempObject.
// Responde 413/500 no primeiro fragmento quando o corpo é recusado.
inline BodyState accumulate_body(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (total == 0 || total > MAX_BODY_BYTES) {
        if (index == 0) request->send(413, "application/json", "{\"status\":\"error\",\"message\":\"Payload muito grande\"}");
        return BodyState::Rejected;
    }

    if (index == 0) {
        request->_tempObject = malloc(total + 1);
        if (request->_tempObject == nullptr) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Memoria insuficiente\"}");
            return BodyState::Rejected;
        }
    }

    if (request->_tempObject == nullptr || index + len > total) return BodyState::Rejected;

    memcpy((uint8_t*)request->_tempObject + index, data, len);
    if (index + len < total) return BodyState::Incomplete;

    ((uint8_t*)request->_tempObject)[total] = '\0';
    return BodyState::Complete;
}

inline String processor(const String& var) {
    if(var == "SSID_VALUE") return (WiFi.status() == WL_CONNECTED) ? html_escape(WiFi.SSID()) : String("Desconectado");
    if(var == "IP_VALUE")   return (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "0.0.0.0";
    if(var == "MAC_VALUE")  return WiFi.macAddress();

    if(var == "MODULE_VALUE")         return physicalAccessControl.modelBoardESP();
    if (var == "TOTAL_RAM_VALUE")      return String((uint32_t)physicalAccessControl.total_ram_kb()) + " KB";
    if (var == "FLASH_SIZE_VALUE")     return String((uint32_t)physicalAccessControl.flash_size_mb()) + " MB";
    if (var == "MENOR_RAM_SIZE_VALUE") return String((uint32_t)physicalAccessControl.min_free_ram_kb()) + " KB";
    if (var == "SKETCH_SIZE_VALUE")    return String((uint32_t)physicalAccessControl.sketch_size_kb()) + " KB";
    if(var == "WEBSERVER_STATE") return g_webserver_state ? "checked" : "";

    if(var == "WAKE_TIME_VALUE" || var == "SLEEP_TIME_VALUE") {
        String value;
        Hours_Time* clock = Hours_Time::instance();
        if (clock != nullptr) {
            if (var == "WAKE_TIME_VALUE") clock->getHoursWakeon(value);
            else clock->getHoursSleep(value);
        }
        return value;
    }

    return String("");
}

inline void handleDateTime(AsyncWebServerRequest *request) {
    time_t now = time(nullptr);

    // Antes da primeira sincronização NTP o relógio fica próximo de 1970
    if (now < 1700000000) {
        request->send(200, "application/json", "{\"date\":\"--/--/----\",\"time\":\"--:--:--\",\"synced\":false}");
        return;
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char dateString[11];
    char hourString[9];

    strftime(dateString, sizeof(dateString), "%d/%m/%Y", &timeinfo);
    strftime(hourString, sizeof(hourString), "%H:%M:%S", &timeinfo);

    String responseJson = "{\"date\":\"";
    responseJson += dateString;
    responseJson += "\",\"time\":\"";
    responseJson += hourString;
    responseJson += "\",\"synced\":true}";

    request->send(200, "application/json", responseJson);
}

// Atualiza o display (LVGL, somente na task principal) e os clientes WebSocket
inline void update_display_and_ws() {
    static TelemetryData snapshot;
    bool refresh_display;
    bool refresh_ws;

    {
        std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);
        refresh_display = g_telemetry.updated;
        refresh_ws = g_ws_dirty;
        g_telemetry.updated = false;
        g_ws_dirty = false;
        if (refresh_display) copy_display_fields(snapshot, g_telemetry);
    }

    if (refresh_ws && ws.count() > 0) {
        if (ws.availableForWriteAll()) ws.textAll(build_json_string());
        else g_ws_dirty = true; // Fila cheia: tenta de novo no próximo ciclo
    }

    if (!refresh_display) return;

    // Alinhamento exato de arredondamento com o JavaScript (Math.round)
    int ram_pct = (snapshot.ram_total_bytes > 0)
        ? (int)(((snapshot.ram_used_bytes * 100ULL) + (snapshot.ram_total_bytes / 2ULL)) / snapshot.ram_total_bytes)
        : 0;

    int disk_pct = (snapshot.disk_total_bytes > 0)
        ? (int)(((snapshot.disk_used_bytes * 100ULL) + (snapshot.disk_total_bytes / 2ULL)) / snapshot.disk_total_bytes)
        : 0;

    update_telemetry_data(
        snapshot.host,
        snapshot.os_name,
        snapshot.kernel,
        snapshot.temp,
        snapshot.cpu_load,
        ram_pct,
        disk_pct,
        snapshot.uptime,
        snapshot.logo_url,
        (float)snapshot.cpu_cores
    );
    update_status(snapshot.online);
}

inline bool parse_and_reply(AsyncWebServerRequest *request, size_t total) {
    ParseResult result = parse_telemetry_json_ex((uint8_t*)request->_tempObject, total);

    switch (result) {
        case ParseResult::Ok:
            request->send(200, "application/json", "{\"status\":\"sucess\"}");
            return true;
        case ParseResult::HostLimit:
            request->send(507, "application/json", "{\"status\":\"error\",\"message\":\"Limite de hosts atingido\"}");
            return false;
        case ParseResult::InvalidId:
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"ID invalido\"}");
            return false;
        default:
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON Invalido\"}");
            return false;
    }
}

inline void on_ws_event(AsyncWebSocket *server_ptr, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (!g_webserver_state) return;

    if (type == WS_EVT_CONNECT) {
        client->text(build_json_string());
    } else if (type == WS_EVT_DATA) {
        // Com token configurado, a escrita só é aceita via HTTP autenticado
        if (strlen(API_TOKEN) != 0) return;

        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT && len <= MAX_BODY_BYTES) {
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

    if (!is_authorized(request)) {
        if (index == 0) request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Nao autorizado\"}");
        return;
    }

    if (accumulate_body(request, data, len, index, total) != BodyState::Complete) return;

    parse_and_reply(request, total);
}

inline void handle_post_config(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!is_authorized(request)) {
        if (index == 0) request->send(401, "application/json", "{\"status\":\"error\",\"message\":\"Nao autorizado\"}");
        return;
    }

    if (accumulate_body(request, data, len, index, total) != BodyState::Complete) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, (uint8_t*)request->_tempObject, total);

    if (error) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON Invalido\"}");
        return;
    }

    // Valida tudo antes de aplicar qualquer alteração
    bool has_wake = doc["wake_time"].is<const char*>();
    bool has_sleep = doc["sleep_time"].is<const char*>();
    if ((has_wake && !Hours_Time::isValidTimeFormat(doc["wake_time"].as<const char*>())) ||
        (has_sleep && !Hours_Time::isValidTimeFormat(doc["sleep_time"].as<const char*>()))) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Horario invalido (use HH:MM)\"}");
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

    if (has_wake || has_sleep) {
        // Hours_Time é a fonte única dos horários (os setters são protegidos por mutex)
        Hours_Time* clock = Hours_Time::instance();
        if (clock != nullptr) {
            if (has_wake) clock->setHoursWakeon(doc["wake_time"].as<String>());
            if (has_sleep) clock->setHoursSleep(doc["sleep_time"].as<String>());
        }
    }

    g_config_dirty = true; // Persistido na task principal

    request->send(200, "application/json", "{\"status\":\"sucess\"}");
}

inline void setup_web_server() {
    // Horários: valores do relógio -> sobrescritos pelos salvos na NVS (aplicados direto no Hours_Time)
    load_config_from_nvs();

    // CORS só é habilitado quando uma origem é definida em CORS_ALLOW_ORIGIN (SecurityConfig.h)
    if (strlen(CORS_ALLOW_ORIGIN) > 0) {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", CORS_ALLOW_ORIGIN);
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    }

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
        request->send(200, "text/html", index_html); // Transmite direto da flash, sem copiar para o heap
    });
    
    server.on("/datetime", HTTP_GET, handleDateTime);

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", config_html, processor);
    });

    server.on("/api/autodiscovery", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });
    server.on("/api/autodiscovery", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!g_webserver_state) {
            request->send(503, "application/json", "{\"status\":\"paused\",\"message\":\"Serviço suspenso\"}");
            return;
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/telemetry", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });
    server.on("/api/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request) { request->send(200); });

    // Requisições sem corpo não passam pelo handler de body: respondem aqui para não ficarem penduradas
    server.on("/api/telemetry", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->contentLength() == 0) request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Corpo vazio\"}");
    }, NULL, handle_post_telemetry);
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->contentLength() == 0) request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Corpo vazio\"}");
    }, NULL, handle_post_config);

    server.begin();
    Serial.println("Servidor HTTP Async Iniciado!");
}

inline void process_web_server_tasks() {
    ws.cleanupClients();

    // Persiste na task principal as alterações de /api/config (a escrita na NVS não roda na task do AsyncTCP)
    if (g_config_dirty) {
        g_config_dirty = false;
        save_config_to_nvs();
    }

    if (!g_webserver_state) return;

    uint32_t now = millis();
    static uint32_t last_ws_send_ms = 0;

    {
        std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);

        if (!g_telemetry_list.empty()) {
            uint32_t timeout_threshold_ms = DISPLAY_ROTATION_INTERVAL_MS * 2 * g_telemetry_list.size();
            bool list_changed = false;

            for (auto it = g_telemetry_list.begin(); it != g_telemetry_list.end(); ) {
                uint32_t age_ms = now - it->last_seen_ms;

                if (age_ms > timeout_threshold_ms) {
                    it = g_telemetry_list.erase(it);
                    list_changed = true;
                } else {
                    // Sem payload recente: marca como offline antes de remover da lista
                    if (it->online && age_ms > HOST_OFFLINE_TIMEOUT_MS) {
                        it->online = false;
                        list_changed = true;
                    }
                    ++it;
                }
            }

            if (list_changed) {
                invalidate_json_cache();

                if (g_telemetry_list.empty()) {
                    g_current_display_index = 0;
                    g_telemetry = TelemetryData();
                    g_telemetry.updated = true;
                } else {
                    if (g_current_display_index >= g_telemetry_list.size()) g_current_display_index = 0;
                    copy_display_fields(g_telemetry, g_telemetry_list[g_current_display_index]);
                    g_telemetry.updated = true;
                }
            }
        }

        if (!g_telemetry_list.empty()) {
            if (now - g_last_host_switch_ms >= DISPLAY_ROTATION_INTERVAL_MS) {
                g_last_host_switch_ms = now;
                g_current_display_index = (g_current_display_index + 1) % g_telemetry_list.size();
                copy_display_fields(g_telemetry, g_telemetry_list[g_current_display_index]);
                g_telemetry.updated = true;
            }
        }
    }

    if ((g_telemetry.updated || g_ws_dirty) && (now - last_ws_send_ms >= WS_SEND_INTERVAL_MS)) {
        last_ws_send_ms = now;
        update_display_and_ws();
    }

    yield();
}

#endif // SERVIDORWEB_H