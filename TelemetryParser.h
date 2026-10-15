#ifndef TELEMETRYPARSER_H
#define TELEMETRYPARSER_H

#include <Arduino.h>
#include "TelemetryData.h"

enum class ParseResult {
    Ok,
    InvalidJson,
    InvalidId,
    HostLimit
};

// Retorna o host existente (por id ou nome) ou cria um novo; nullptr se MAX_TELEMETRY_HOSTS foi atingido.
// Deve ser chamado com g_telemetry_mutex adquirido (o mutex é recursivo).
TelemetryData* get_or_create_host(const char* id_or_name);

// Interpreta o payload JSON de um coletor e atualiza o host correspondente.
ParseResult parse_telemetry_json_ex(uint8_t *data, size_t len);
bool parse_telemetry_json(uint8_t *data, size_t len);

// JSON enviado aos clientes WebSocket (reaproveitado até que algum host mude).
String build_json_string();
void invalidate_json_cache();

#endif // TELEMETRYPARSER_H