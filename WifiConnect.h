#ifndef WIFICONNECT_H
#define WIFICONNECT_H

#include <WiFi.h>

// Estado da máquina de conexão assíncrona
enum StatusRede {
    REDE_DESCONECTADA,
    REDE_CONECTANDO,
    REDE_CONECTADA
};

class WifiConnect {
private:
    const char* ssid;
    const char* password;

    // Gerenciamento da rede de backup
    String backupSsid;
    String backupPass;
    bool ProcessoDeBackup = false;

    // Controle do ciclo de tentativas e tempo (Backoff Exponencial)
    StatusRede _estado = REDE_DESCONECTADA;
    unsigned long _ultimaTentativaMs = 0;
    unsigned long _intervaloReconexao = 2000;      // Começa em 2 segundos
    const unsigned long _intervaloMaximo = 60000;   // Limite máximo de 1 minuto

    void registrarEventos();

public:
    WifiConnect(const char* ssid, const char* password);

    // Inicialização da interface Wi-Fi sem bloquear o boot
    void connections_Wifi();

    // Máquina de estados executada continuamente no loop()
    void loop();

    // Utilitários de status e controle do rádio
    bool connections_status();
    void diconnectRede();
    void wifiOff();
    void searchRedes();
    String getEncryptionName(uint8_t encryptionType);
};

#endif // WIFICONNECT_H