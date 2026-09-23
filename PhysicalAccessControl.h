#ifndef PHYSICALACCESSCONTROL_H
#define PHYSICALACCESSCONTROL_H

#include <Arduino.h>

class PhysicalAccessControl {
  public:
    PhysicalAccessControl();
    
    // Métodos de Informação do Sistema
    String modelBoardESP();
    
    // Métodos de Memória (Retornam valores numéricos em KB/MB para facilitar cálculos)
    uint32_t free_ram_kb();
    uint32_t total_ram_kb();
    uint32_t flash_size_mb();
    uint32_t min_free_ram_kb();
    uint32_t sketch_size_kb();
    
    // Mapeamento de GPIOs em JSON
    String pinGPIO();
};

#endif // PHYSICALACCESSCONTROL_H