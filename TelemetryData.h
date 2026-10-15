#ifndef TELEMETRYDATA_H
#define TELEMETRYDATA_H

#include <Arduino.h>
#include <vector>
#include <mutex>
#include <string.h>

// Estrutura para os Discos Físicos Individuais (hardware_discos)
struct DiskInfo {
    char device[32] = "";
    char model[64] = "Desconhecido";
    uint64_t total_bytes = 0;
    uint64_t usado_bytes = 0;
    uint64_t livre_bytes = 0;
    char health_status[16] = "UNKNOWN";
};

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
    uint8_t cpu_cores = 4;   // Núcleos informados pelo coletor (padrão: 4)

    // Dados Brutos em Bytes
    uint64_t ram_total_bytes = 0;
    uint64_t ram_used_bytes  = 0;
    uint64_t disk_total_bytes = 0;
    uint64_t disk_used_bytes  = 0;

    // Lista de Discos Físicos do Sistema
    std::vector<DiskInfo> hardware_discos;

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

// Limites e temporizações
constexpr size_t   MAX_TELEMETRY_HOSTS      = 16;                            // Evita esgotar o heap com hosts forjados
constexpr size_t   MAX_BODY_BYTES           = 8192;                          // Tamanho máximo do corpo de um POST
constexpr uint32_t HOST_OFFLINE_TIMEOUT_MS  = DISPLAY_ROTATION_INTERVAL_MS;  // Sem payload por este tempo => offline
constexpr uint32_t WS_SEND_INTERVAL_MS      = 500;                           // Intervalo mínimo entre envios WebSocket

// Protege g_telemetry, g_telemetry_list e os índices do carrossel.
// Os handlers HTTP/WebSocket rodam na task do AsyncTCP e o loop() na task principal.
inline std::recursive_mutex g_telemetry_mutex;

// Indica que o JSON dos clientes WebSocket precisa ser reenviado (qualquer host mudou)
inline volatile bool g_ws_dirty = false;

// Copia apenas os campos usados pelo display (sem alocar o vetor de discos)
inline void copy_display_fields(TelemetryData& dst, const TelemetryData& src) {
    memcpy(dst.id, src.id, sizeof(dst.id));
    memcpy(dst.ip, src.ip, sizeof(dst.ip));
    memcpy(dst.mac, src.mac, sizeof(dst.mac));
    memcpy(dst.host, src.host, sizeof(dst.host));
    memcpy(dst.os_name, src.os_name, sizeof(dst.os_name));
    memcpy(dst.kernel, src.kernel, sizeof(dst.kernel));
    memcpy(dst.uptime, src.uptime, sizeof(dst.uptime));
    memcpy(dst.logo_url, src.logo_url, sizeof(dst.logo_url));
    memcpy(dst.datetime, src.datetime, sizeof(dst.datetime));
    dst.temp = src.temp;
    dst.cpu_load = src.cpu_load;
    dst.cpu_cores = src.cpu_cores;
    dst.ram_total_bytes = src.ram_total_bytes;
    dst.ram_used_bytes = src.ram_used_bytes;
    dst.disk_total_bytes = src.disk_total_bytes;
    dst.disk_used_bytes = src.disk_used_bytes;
    dst.online = src.online;
    dst.epoch_timestamp = src.epoch_timestamp;
    dst.last_seen_ms = src.last_seen_ms;
}

#endif // TELEMETRYDATA_H