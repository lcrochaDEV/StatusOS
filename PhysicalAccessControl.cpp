#include <ArduinoJson.h>
#include "PhysicalAccessControl.h"

// Configuração automática de bibliotecas baseada na placa
#if defined(ESP8266)
  inline String MODELO_VALUE() { return String("ESP8266 ESP-01"); }
#elif defined(ESP32)
  inline String MODELO_VALUE() { return String(ESP.getChipModel()); }
#else
  inline String MODELO_VALUE() { return String("Desconhecido"); }
#endif

PhysicalAccessControl::PhysicalAccessControl() {}

String PhysicalAccessControl::modelBoardESP() { return MODELO_VALUE(); }

// Memória RAM Livre em KB
uint32_t PhysicalAccessControl::free_ram_kb() { 
    return ESP.getFreeHeap() / 1024; 
}

// Memória RAM Total em KB
uint32_t PhysicalAccessControl::total_ram_kb() { 
  #if defined(ESP8266)
    return 80; // O ESP8266 possui aproximadamente 80KB de RAM total para o usuário
  #elif defined(ESP32)
    return ESP.getHeapSize() / 1024; 
  #else
    return 0;
  #endif
}

// Tamanho da Memória Flash em MB
uint32_t PhysicalAccessControl::flash_size_mb() { 
  #if defined(ESP8266)
    return ESP.getFlashChipRealSize() / (1024 * 1024);
  #elif defined(ESP32)
    return ESP.getFlashChipSize() / (1024 * 1024);
  #else
    return 0;
  #endif
}

// Menor nível de RAM livre registrado desde o boot (Watermark) em KB
uint32_t PhysicalAccessControl::min_free_ram_kb() { 
  #if defined(ESP32)
    return ESP.getMinFreeHeap() / 1024;
  #else
    return ESP.getFreeHeap() / 1024;
  #endif
}

// Tamanho do Firmware (Sketch) compilado em KB
uint32_t PhysicalAccessControl::sketch_size_kb() { 
    return ESP.getSketchSize() / 1024; 
}

// Retorna um JSON dinâmico com quantos GPIOs cada placa tiver
String PhysicalAccessControl::pinGPIO() {
  JsonDocument doc;
  JsonObject gpios = doc["gpios"].to<JsonObject>();

  #if defined(ESP8266)
    // Configuração para módulos menores ou NodeMCU
    gpios["GPIO0"] = 0; //D2
    gpios["GPIO2"] = 2; //D4
  #elif defined(ESP32)
    // O ESP32 tem muito mais versatilidade de pinos
    gpios["GPIO0"] = 0;
    gpios["GPIO2"] = 2;
    gpios["GPIO4"] = 4;
    gpios["GPIO5"] = 5;
    gpios["GPIO18"] = 18;
    gpios["GPIO19"] = 19;
    gpios["GPIO21"] = 21;
    gpios["GPIO22"] = 22;
  #endif

  String output;
  serializeJson(doc, output);
  return output;
}