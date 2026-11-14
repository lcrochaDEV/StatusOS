#ifndef SDDATA_H
#define SDDATA_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

class SDData {
  public:
    // Construtor: se nenhum caminho for passado, assume "/data/config.json"
    SDData(const char* filename = nullptr);

    // Inicialização e gerenciamento de diretórios
    void sdbegin();
    bool createFolder(const char * path);
    void listDir(const char * dirname, uint8_t levels);

    // Métodos CRUD Genéricos (Aceitam qualquer estrutura JsonDocument)
    void createJSON(JsonDocument& doc);
    bool readJSON(JsonDocument& doc);
    void updateJSON(const char* chave, const char* novoValor);
    void deleteArquivo();

    // Métodos de Negócio: Manipulação da estrutura aninhada 'wakeonMode'
    bool saveWakeonSettings(const String& name, const String& wakeon, const String& sleep);
    bool loadWakeonSettings(String& outName, String& outWakeon, String& outSleep);

    // Método para imprimir o JSON formatado no Monitor Serial
    void printJSON();
    // Metodo para ler uma animação local de contingência (ex: /data/fallback.json) gravada no mesmo formato do MongoDB.
    bool readLocalFallbackFrame(int index, String& outHexData);

    // Verifica se o SD está disponível
    bool isReady() const;

  protected:
    const char* filename;
};

inline SDData sdData;

#endif
