#include "SDData.h"

SDData::SDData(const char* filename) {
    this->filename = (filename != nullptr) ? filename : "/data/config.json";
}

void SDData::sdbegin() {
    if(!SD.begin()){
        Serial.println("Falha ao montar o Cartão SD!");
        return;
    }
    Serial.println("Cartao SD inicializado com sucesso!");
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("Nenhum cartao SD encontrado ou tipo desconhecido.");
    } else {
        Serial.print("Tipo de cartao SD: ");
        if (cardType == CARD_MMC) Serial.println("MMC");
        else if (cardType == CARD_SD) Serial.println("SDSC");
        else if (cardType == CARD_SDHC) Serial.println("SDHC");
        else Serial.println("DESCONHECIDO");

        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("Tamanho do cartao: %lluMB\n", cardSize);
    }
    createFolder("/data");
}

bool SDData::createFolder(const char * path) {
    if (SD.exists(path)) return true;
    if (SD.mkdir(path)) {
        Serial.printf("Pasta '%s' criada!\n", path);
        return true;
    }
    return false;
}

void SDData::listDir(const char * dirname, uint8_t levels) {
    File root = SD.open(dirname);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory() && levels > 0) {
            listDir(file.path(), levels - 1);
        } else {
            Serial.printf(" FILE: %s SIZE %d\n", file.name(), (int)file.size());
        }
        file = root.openNextFile();
    }
}

void SDData::createJSON(JsonDocument& doc) {
    // Segurança: Escrita atômica simulada (opcional: criar .tmp e renomear)
    File file = SD.open(filename, "w");
    if (!file) {
        Serial.println("Erro ao abrir arquivo para escrita");
        return;
    }
    if (serializeJson(doc, file) == 0) {
        Serial.println("Falha ao serializar JSON");
    }
    file.close();
}

bool SDData::readJSON(JsonDocument& doc) {
    if (!SD.exists(filename)) return false;
    File file = SD.open(filename, "r");
    if (!file) return false;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    return !error;
}

void SDData::updateJSON(const char* chave, const char* novoValor) {
    JsonDocument doc;
    readJSON(doc);
    doc[chave] = novoValor;
    createJSON(doc);
}

void SDData::deleteArquivo() {
    if (SD.exists(filename)) SD.remove(filename);
}

bool SDData::saveWakeonSettings(const String& name, const String& wakeon, const String& sleep) {
    JsonDocument doc;
    readJSON(doc);
    JsonObject wo = doc["wakeonMode"].to<JsonObject>();
    wo["name"] = name;
    wo["wake"] = wakeon;
    wo["sleep"] = sleep;
    createJSON(doc);
    return true;
}

bool SDData::loadWakeonSettings(String& outName, String& outWakeon, String& outSleep) {
    JsonDocument doc;
    if (!readJSON(doc)) return false;
    outName = doc["wakeonMode"]["name"].as<String>();
    outWakeon = doc["wakeonMode"]["wake"].as<String>();
    outSleep = doc["wakeonMode"]["sleep"].as<String>();
    return true;
}

void SDData::printJSON() {
    JsonDocument doc;
    if (readJSON(doc)) serializeJsonPretty(doc, Serial);
}

bool SDData::readLocalFallbackFrame(int index, String& outHexData) {
    // Implementação simplificada para o index
    return false;
}

bool SDData::isReady() const {
    return SD.cardType() != CARD_NONE;
}
