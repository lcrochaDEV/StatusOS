#include <ArduinoJson.h>
#include "PhysicalAccessControl.h"

// Configuração automática de bibliotecas baseada na placa
#if defined(ESP8266)
  inline String MODELO_VALUE() { return String("ESP8266 ESP-01"); }
#elif defined(ESP32)
  inline String MODELO_VALUE() { return String(ESP.getChipModel()); }
#endif

PhysicalAccessControl::PhysicalAccessControl() {}

String PhysicalAccessControl::modelBoardESP() { return MODELO_VALUE(); }

String PhysicalAccessControl::total_ram() { 
  #if defined(ESP8266)
    // No ESP8266 usamos a soma para estimar a RAM total livre
    return String((ESP.getFreeHeap() + 32000) / 1024) + " KB"; 
  #else
    // No ESP32 existe função nativa para o tamanho total do Heap
    return String(ESP.getHeapSize() / 1024) + " KB"; 
  #endif
}

String PhysicalAccessControl::flash_size() { 
  #if defined(ESP8266)
    return String(ESP.getFlashChipRealSize() / (1024 * 1024)) + " MB";
  #else
    return String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB";
  #endif
}

String PhysicalAccessControl::menor_ram_size() { 
  return String((unsigned long)ESP.getFreeHeap() / 1024) + " KB"; 
}

String PhysicalAccessControl::sketch_Size() { 
  return String((unsigned long)ESP.getSketchSize() / 1024) + " KB"; 
}

// Retorna um JSON dinâmico com quantos GPIOs cada placa tiver
String PhysicalAccessControl::pinGPIO() {
  JsonDocument doc;
  JsonObject gpios = doc.createNestedObject("gpios");

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