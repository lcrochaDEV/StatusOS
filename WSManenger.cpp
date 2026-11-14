#include "WSManenger.h"

WSManenger wsManager;

WSManenger::WSManenger(const char* serverPath) : _server(serverPath) {}

// ===================================================================== Servidor

void WSManenger::attachServer(AsyncWebServer& server) {
    if (_serverAttached) return;

    _server.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
                           void* arg, uint8_t* data, size_t len) {
        handleServerEvent(client, type, arg, data, len);
    });
    server.addHandler(&_server);
    _serverAttached = true;
}

void WSManenger::onServerConnect(ServerConnectCb cb) { _onServerConnect = std::move(cb); }
void WSManenger::onServerText(ServerTextCb cb)       { _onServerText = std::move(cb); }

void WSManenger::setServerEnabled(bool enabled) {
    if (_serverEnabled == enabled) return;
    _serverEnabled = enabled;
    if (!_serverEnabled) _server.closeAll();
}

bool WSManenger::isServerEnabled() const { return _serverEnabled; }

void WSManenger::setServerAcceptIncoming(bool accept) { _acceptIncoming = accept; }
void WSManenger::setServerMaxIncomingBytes(size_t maxBytes) { _maxIncomingBytes = maxBytes; }

size_t WSManenger::serverClientCount() { return _server.count(); }
bool   WSManenger::serverCanSendAll()  { return _server.availableForWriteAll(); }

bool WSManenger::broadcastText(const String& message) {
    if (_server.count() == 0) return true;
    if (!_server.availableForWriteAll()) return false; // Fila cheia: o chamador tenta no próximo ciclo
    _server.textAll(message);
    return true;
}

void WSManenger::closeAllServerClients() { _server.closeAll(); }

void WSManenger::serverCleanup() { _server.cleanupClients(); }

void WSManenger::handleServerEvent(AsyncWebSocketClient* client, AwsEventType type,
                                   void* arg, uint8_t* data, size_t len) {
    if (!_serverEnabled) return;

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

void WSManenger::beginClientSSL(const char* host, uint16_t port, const char* path,
                                uint32_t reconnectMs,
                                uint32_t heartbeatPingMs,
                                uint32_t heartbeatPongTimeoutMs,
                                uint8_t heartbeatDisconnectAfter) {
    _clientDown = false;

    _client.beginSSL(host, port, path);
    _client.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleClientEvent(type, payload, length);
    });
    _client.setReconnectInterval(reconnectMs);
    _client.enableHeartbeat(heartbeatPingMs, heartbeatPongTimeoutMs, heartbeatDisconnectAfter);

    _clientStarted = true;
}

void WSManenger::onClientEvent(ClientEventCb cb) { _onClientEvent = std::move(cb); }

void WSManenger::clientLoop() {
    if (_clientStarted) _client.loop();
}

void WSManenger::disconnectClient() {
    if (_clientStarted) _client.disconnect();
    _clientStarted = false;
    _clientDown = false;
}

bool WSManenger::sendClientText(const char* message) {
    if (!_clientStarted) return false;
    return _client.sendTXT(message);
}

bool WSManenger::isClientStarted() const { return _clientStarted; }
bool WSManenger::isClientConnected()     { return _clientStarted && _client.isConnected(); }
bool WSManenger::isClientDown() const    { return _clientDown; }

void WSManenger::handleClientEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.println("[ws] cliente conectado");
    } else if (type == WStype_DISCONNECTED || type == WStype_ERROR) {
        if (_clientStarted) {
            if (!_clientDown) Serial.println("[ws] cliente indisponivel");
            _clientDown = true;
        }
    } else if (type == WStype_TEXT) {
        _clientDown = false;
    }

    if (_onClientEvent) _onClientEvent(type, payload, length);
}
