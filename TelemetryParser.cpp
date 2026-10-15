#include "TelemetryParser.h"
#include <ArduinoJson.h>

static const size_t MAX_DISKS_PER_HOST = 16;

static String g_json_cache;
static bool g_json_dirty = true;

void invalidate_json_cache() {
    std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);
    g_json_dirty = true;
    g_ws_dirty = true;
}

// Copia uma string JSON para um buffer fixo, sempre terminando em '\0'
static void copy_json_string(char* dst, size_t capacity, JsonVariantConst value) {
    if (!value.is<const char*>()) return;
    strncpy(dst, value.as<const char*>(), capacity - 1);
    dst[capacity - 1] = '\0';
}

// Lê um valor em bytes aceitando as variações *_bytes, *_kb e *_mb (kb/mb opcionais)
static bool read_bytes(JsonVariantConst obj, const char* bytesKey, const char* kbKey, const char* mbKey, uint64_t& out) {
    JsonVariantConst value = obj[bytesKey];
    if (!value.isNull()) { out = value.as<uint64_t>(); return true; }

    if (kbKey != nullptr) {
        value = obj[kbKey];
        if (!value.isNull()) { out = value.as<uint64_t>() * 1024ULL; return true; }
    }
    if (mbKey != nullptr) {
        value = obj[mbKey];
        if (!value.isNull()) { out = value.as<uint64_t>() * 1024ULL * 1024ULL; return true; }
    }
    return false;
}

static void read_cpu_cores(JsonVariantConst obj, TelemetryData* host) {
    JsonVariantConst cores = obj["cpu_cores"];
    if (cores.isNull()) return;
    unsigned value = cores.as<unsigned>();
    if (value >= 1 && value <= 255) host->cpu_cores = (uint8_t)value;
}

TelemetryData* get_or_create_host(const char* id_or_name) {
    std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);

    for (auto& item : g_telemetry_list) {
        if (strcmp(item.id, id_or_name) == 0 || strcmp(item.host, id_or_name) == 0) {
            return &item;
        }
    }

    if (g_telemetry_list.size() >= MAX_TELEMETRY_HOSTS) return nullptr;

    TelemetryData new_host;
    strncpy(new_host.id, id_or_name, sizeof(new_host.id) - 1);
    strncpy(new_host.host, id_or_name, sizeof(new_host.host) - 1);
    g_telemetry_list.push_back(new_host);
    return &g_telemetry_list.back();
}

String build_json_string() {
    std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);
    if (!g_json_dirty) return g_json_cache;

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

        obj["ram_total_bytes"]  = item.ram_total_bytes;
        obj["ram_used_bytes"]   = item.ram_used_bytes;
        obj["disk_total_bytes"] = item.disk_total_bytes;
        obj["disk_used_bytes"]  = item.disk_used_bytes;

        JsonObject metricasObj = obj["metricas"].to<JsonObject>();
        metricasObj["cpu_temp"] = item.temp;
        metricasObj["cpu_load_1m"] = item.cpu_load;
        metricasObj["cpu_cores"] = item.cpu_cores;

        JsonObject memObj = metricasObj["memoria"].to<JsonObject>();
        memObj["total_bytes"] = item.ram_total_bytes;
        memObj["usada_bytes"] = item.ram_used_bytes;

        JsonObject diskObj = metricasObj["disco"].to<JsonObject>();
        diskObj["total_bytes"] = item.disk_total_bytes;
        diskObj["usado_bytes"] = item.disk_used_bytes;

        JsonArray hwDisksArr = metricasObj["hardware_discos"].to<JsonArray>();
        for (const auto& d : item.hardware_discos) {
            JsonObject diskItem = hwDisksArr.add<JsonObject>();
            diskItem["device"] = d.device;
            diskItem["model"] = d.model;
            diskItem["total_bytes"] = d.total_bytes;
            diskItem["usado_bytes"] = d.usado_bytes;
            diskItem["livre_bytes"] = d.livre_bytes;
            diskItem["health_status"] = d.health_status;
        }

        obj["logo_url"] = item.logo_url;
        obj["online"]   = item.online;
    }

    g_json_cache = "";
    g_json_cache.reserve(measureJson(doc));
    serializeJson(doc, g_json_cache);
    g_json_dirty = false;
    return g_json_cache;
}

ParseResult parse_telemetry_json_ex(uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) return ParseResult::InvalidJson;

    JsonVariant root = doc.as<JsonVariant>();
    if (!root.is<JsonObject>()) return ParseResult::InvalidJson;

    // O coletor precisa se identificar (id, servidor ou host); sem isso o payload é recusado
    const char* host_id = nullptr;
    if (doc["id"].is<const char*>()) host_id = doc["id"];
    else if (doc["servidor"].is<const char*>()) host_id = doc["servidor"];
    else if (doc["host"].is<const char*>()) host_id = doc["host"];

    if (host_id == nullptr) return ParseResult::InvalidId;

    size_t host_id_len = strlen(host_id);
    if (host_id_len == 0 || host_id_len >= sizeof(TelemetryData::id)) return ParseResult::InvalidId;

    std::lock_guard<std::recursive_mutex> lock(g_telemetry_mutex);

    TelemetryData* target_host = get_or_create_host(host_id);
    if (target_host == nullptr) return ParseResult::HostLimit;
    target_host->last_seen_ms = millis();

    copy_json_string(target_host->id, sizeof(target_host->id), doc["id"]);
    copy_json_string(target_host->ip, sizeof(target_host->ip), doc["ip"]);
    copy_json_string(target_host->mac, sizeof(target_host->mac), doc["mac"]);

    if (doc["servidor"].is<const char*>()) copy_json_string(target_host->host, sizeof(target_host->host), doc["servidor"]);
    else if (doc["host"].is<const char*>()) copy_json_string(target_host->host, sizeof(target_host->host), doc["host"]);

    copy_json_string(target_host->datetime, sizeof(target_host->datetime), doc["datetime"]);
    if (!doc["epoch_timestamp"].isNull()) target_host->epoch_timestamp = doc["epoch_timestamp"].as<uint64_t>();
    copy_json_string(target_host->uptime, sizeof(target_host->uptime), doc["uptime"]);
    copy_json_string(target_host->logo_url, sizeof(target_host->logo_url), doc["logo_url"]);

    if (doc["sistema_operacional"].is<JsonObject>()) {
        JsonObject so = doc["sistema_operacional"];
        copy_json_string(target_host->os_name, sizeof(target_host->os_name), so["nome"]);
        copy_json_string(target_host->kernel, sizeof(target_host->kernel), so["kernel"]);
        copy_json_string(target_host->logo_url, sizeof(target_host->logo_url), so["logo_url"]);
    } else {
        copy_json_string(target_host->os_name, sizeof(target_host->os_name), doc["os_name"]);
        copy_json_string(target_host->kernel, sizeof(target_host->kernel), doc["kernel"]);
    }

    if (doc["metricas"].is<JsonObject>()) {
        JsonObject metricas = doc["metricas"];
        if (!metricas["cpu_temp"].isNull()) target_host->temp = metricas["cpu_temp"].as<float>();
        if (!metricas["cpu_load_1m"].isNull()) target_host->cpu_load = metricas["cpu_load_1m"].as<float>();
        read_cpu_cores(metricas, target_host);

        if (metricas["memoria"].is<JsonObject>()) {
            JsonObject mem = metricas["memoria"];
            read_bytes(mem, "total_bytes", "total_kb", "total_mb", target_host->ram_total_bytes);
            read_bytes(mem, "usada_bytes", "usada_kb", "usada_mb", target_host->ram_used_bytes);
        }

        if (metricas["disco"].is<JsonObject>()) {
            JsonObject dsk = metricas["disco"];
            read_bytes(dsk, "total_bytes", "total_kb", "total_mb", target_host->disk_total_bytes);
            read_bytes(dsk, "usado_bytes", "usado_kb", "usado_mb", target_host->disk_used_bytes);
        }

        // A lista de discos só é substituída quando o coletor a envia
        if (metricas["hardware_discos"].is<JsonArray>()) {
            target_host->hardware_discos.clear();
            JsonArray discosArr = metricas["hardware_discos"].as<JsonArray>();
            for (JsonObject d : discosArr) {
                if (target_host->hardware_discos.size() >= MAX_DISKS_PER_HOST) break;

                DiskInfo disk;
                copy_json_string(disk.device, sizeof(disk.device), d["device"]);
                copy_json_string(disk.model, sizeof(disk.model), d["model"]);

                if (!read_bytes(d, "total_bytes", nullptr, nullptr, disk.total_bytes)) {
                    read_bytes(d, "size_bytes", nullptr, nullptr, disk.total_bytes);
                }

                read_bytes(d, "usado_bytes", nullptr, nullptr, disk.usado_bytes);
                read_bytes(d, "livre_bytes", nullptr, nullptr, disk.livre_bytes);

                copy_json_string(disk.health_status, sizeof(disk.health_status), d["health_status"]);

                target_host->hardware_discos.push_back(disk);
            }
        }
    } else {
        if (!doc["temp"].isNull()) target_host->temp = doc["temp"].as<float>();
        if (!doc["cpu_load"].isNull()) target_host->cpu_load = doc["cpu_load"].as<float>();
        read_cpu_cores(root, target_host);

        read_bytes(root, "ram_total_bytes", "ram_total_kb", nullptr, target_host->ram_total_bytes);
        read_bytes(root, "ram_used_bytes", "ram_used_kb", nullptr, target_host->ram_used_bytes);
        read_bytes(root, "disk_total_bytes", "disk_total_kb", nullptr, target_host->disk_total_bytes);
        read_bytes(root, "disk_used_bytes", "disk_used_kb", nullptr, target_host->disk_used_bytes);
    }

    target_host->online = true;
    target_host->updated = true;

    // O display só acompanha o host que está sendo exibido no momento
    size_t host_index = (size_t)(target_host - g_telemetry_list.data());
    if (g_telemetry_list.size() == 1 || host_index == g_current_display_index) {
        copy_display_fields(g_telemetry, *target_host);
        g_telemetry.updated = true;
    }

    invalidate_json_cache();
    return ParseResult::Ok;
}

bool parse_telemetry_json(uint8_t *data, size_t len) {
    return parse_telemetry_json_ex(data, len) == ParseResult::Ok;
}