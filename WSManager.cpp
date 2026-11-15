#include "WSManager.h"

WSManager wsManager;

WSManager::WSManager(const char* serverPath) : _server(serverPath) {}

// ===================================================================== Servidor

void WSManager::attachServer(AsyncWebServer& server) {
    if (_serverAttached) return;

    _server.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
                           void* arg, uint8_t* data, size_t len) {
        handleServerEvent(client, type, arg, data, len);
    });
    server.addHandler(&_server);
    _serverAttached = true;
}

void WSManager::onServerConnect(ServerConnectCb cb) { _onServerConnect = std::move(cb); }
void WSManager::onServerText(ServerTextCb cb)       { _onServerText = std::move(cb); }

void WSManager::setServerEnabled(bool enabled) {
    if (_serverEnabled == enabled) return;
    _serverEnabled = enabled;
    if (!_serverEnabled) _server.closeAll();
}

bool WSManager::isServerEnabled() const { return _serverEnabled; }

void WSManager::setServerAcceptIncoming(bool accept) { _acceptIncoming = accept; }
void WSManager::setServerMaxIncomingBytes(size_t maxBytes) { _maxIncomingBytes = maxBytes; }

size_t WSManager::serverClientCount() { return _server.count(); }
bool   WSManager::serverCanSendAll()  { return _server.availableForWriteAll(); }

bool WSManager::broadcastText(const String& message) {
    if (_server.count() == 0) return true;
    if (!_server.availableForWriteAll()) return false; // Fila cheia: o chamador tenta no próximo ciclo
    _server.textAll(message);
    return true;
}

void WSManager::closeAllServerClients() { _server.closeAll(); }

void WSManager::serverCleanup() { _server.cleanupClients(); }

void WSManager::handleServerEvent(AsyncWebSocketClient* client, AwsEventType type,
                                   void* arg, uint8_t* data, size_t len) {
    if (!_serverEnabled) {
        // Serviço pausado: recusa novas conexões em vez de deixá-las penduradas até o timeout
        if (type == WS_EVT_CONNECT && client != nullptr) client->close();
        return;
    }

    if (type == WS_EVT_CONNECT) {
        if (_onServerConnect) _onServerConnect(client);
    } else if (type == WS_EVT_DATA) {
        if (!_acceptIncoming || !_onServerText) return;

        // Só mensagens de texto completas, em um único frame
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len &&
            info->opcode == WS_TEXT && len <= _maxIncomingBytes) {
            _onServerText(client, data, len);
        }
    }
}

// ===================================================================== Cliente

void WSManager::beginClientSSL(const char* host, uint16_t port, const char* path,
                                uint32_t reconnectMs,
                                uint32_t heartbeatPingMs,
                                uint32_t heartbeatPongTimeoutMs,
                                uint8_t heartbeatDisconnectAfter) {
    _clientDown = false;
    _clientFailures = 0;
    _reconnectBaseMs = reconnectMs;

    _client.beginSSL(host, port, path);
    _client.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleClientEvent(type, payload, length);
    });
    _client.setReconnectInterval(reconnectMs);
    _client.enableHeartbeat(heartbeatPingMs, heartbeatPongTimeoutMs, heartbeatDisconnectAfter);

    _clientStarted = true;
}

void WSManager::onClientEvent(ClientEventCb cb) { _onClientEvent = std::move(cb); }

void WSManager::clientLoop() {
    if (_clientStarted) _client.loop();
}

void WSManager::disconnectClient() {
    if (_clientStarted) _client.disconnect();
    _clientStarted = false;
    _clientDown = false;
}

bool WSManager::sendClientText(const char* message) {
    if (!_clientStarted) return false;
    return _client.sendTXT(message);
}

bool WSManager::isClientStarted() const { return _clientStarted; }
bool WSManager::isClientConnected()     { return _clientStarted && _client.isConnected(); }
bool WSManager::isClientDown() const    { return _clientDown; }

void WSManager::handleClientEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.println("[ws] cliente conectado");
        _clientFailures = 0;
        _client.setReconnectInterval(_reconnectBaseMs);
    } else if (type == WStype_DISCONNECTED || type == WStype_ERROR) {
        if (_clientStarted) {
            if (!_clientDown) Serial.println("[ws] cliente indisponivel");
            _clientDown = true;

            // Cada tentativa de reconexão TLS bloqueia o loop() e usa ~40 KB de heap.
            // Com o servidor fora do ar, o intervalo dobra a cada falha (até 60 s).
            if (_clientFailures < 4) _clientFailures++;
            uint32_t next = _reconnectBaseMs << _clientFailures;
            if (next > CLIENT_RECONNECT_MAX_MS) next = CLIENT_RECONNECT_MAX_MS;
            _client.setReconnectInterval(next);
        }
    } else if (type == WStype_TEXT) {
        _clientDown = false;
    }

    if (_onClientEvent) _onClientEvent(type, payload, length);
}
