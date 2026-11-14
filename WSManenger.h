#ifndef WSMANENGER_H
#define WSMANENGER_H

#include <Arduino.h>
#include <functional>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsClient.h>

/*
 * WSManenger - ponto único para todas as chamadas WebSocket do StatusOS.
 *
 *  Servidor (AsyncWebSocket, rota "/ws"):  telemetria enviada ao dashboard web
 *      attachServer() / broadcastText() / closeAllServerClients() / serverCleanup()
 *
 *  Cliente (WebSocketsClient, SSL):        stream de preços da Binance
 *      beginClientSSL() / clientLoop() / disconnectClient()
 *
 * Os dados de cada lado (JSON de telemetria, parser da Binance) continuam fora da classe:
 * ela só cuida da conexão, da reconexão/heartbeat e do estado.
 */
class WSManenger {
  public:
    using ServerConnectCb = std::function<void(AsyncWebSocketClient* client)>;
    using ServerTextCb    = std::function<void(AsyncWebSocketClient* client, uint8_t* data, size_t len)>;
    using ClientEventCb   = std::function<void(WStype_t type, uint8_t* payload, size_t length)>;

    explicit WSManenger(const char* serverPath = "/ws");

    // ---------------------------------------------------------------- Servidor
    // Registra o handler "/ws" no servidor HTTP (idempotente).
    void attachServer(AsyncWebServer& server);

    // Chamado quando um navegador conecta (ex.: enviar o JSON atual).
    void onServerConnect(ServerConnectCb cb);

    // Chamado para mensagens de texto completas (um único frame) recebidas dos clientes.
    void onServerText(ServerTextCb cb);

    // false: ignora eventos e derruba os clientes conectados (serviço pausado).
    void setServerEnabled(bool enabled);
    bool isServerEnabled() const;

    // false: mensagens recebidas dos clientes são descartadas (ex.: quando há API_TOKEN).
    void setServerAcceptIncoming(bool accept);

    // Tamanho máximo aceito para uma mensagem recebida.
    void setServerMaxIncomingBytes(size_t maxBytes);

    size_t serverClientCount();
    bool   serverCanSendAll();

    // Envia a todos os clientes. Retorna false se a fila estava cheia (tentar de novo depois);
    // retorna true se enviou ou se não há ninguém conectado.
    bool broadcastText(const String& message);

    void closeAllServerClients();

    // Deve ser chamado periodicamente no loop principal.
    void serverCleanup();

    // ----------------------------------------------------------------- Cliente
    // Abre a conexão (não bloqueante); o loop é processado em clientLoop().
    void beginClientSSL(const char* host, uint16_t port, const char* path,
                        uint32_t reconnectMs = 5000,
                        uint32_t heartbeatPingMs = 15000,
                        uint32_t heartbeatPongTimeoutMs = 3000,
                        uint8_t heartbeatDisconnectAfter = 2);

    void onClientEvent(ClientEventCb cb);

    // Processa a conexão do cliente; deve ser chamado em todo loop().
    void clientLoop();

    void disconnectClient();

    bool sendClientText(const char* message);

    // Foi iniciado com beginClientSSL() e não foi encerrado com disconnectClient().
    bool isClientStarted() const;
    bool isClientConnected();

    // true depois de uma queda/erro; volta a false quando chega a próxima mensagem de texto.
    bool isClientDown() const;

  private:
    AsyncWebSocket _server;
    bool   _serverAttached  = false;
    bool   _serverEnabled   = true;
    bool   _acceptIncoming  = true;
    size_t _maxIncomingBytes = 1024;
    ServerConnectCb _onServerConnect;
    ServerTextCb    _onServerText;

    WebSocketsClient _client;
    bool _clientStarted = false;
    bool _clientDown    = false;
    ClientEventCb _onClientEvent;

    void handleServerEvent(AsyncWebSocketClient* client, AwsEventType type,
                           void* arg, uint8_t* data, size_t len);
    void handleClientEvent(WStype_t type, uint8_t* payload, size_t length);
};

// Instância única usada por servidorweb.h, StatusOS.ino e ui_crypto_dashboard.h
extern WSManenger wsManager;

#endif // WSMANENGER_H
