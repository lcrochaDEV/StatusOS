#ifndef PS1_STARTUP_H
#define PS1_STARTUP_H

#include <Arduino.h>
#include <pgmspace.h> // Importante para o PROGMEM

#define PS1_STARTUP_HEIGHT_H 240
#define PS1_STARTUP_WIDTH_H  320

// Adicione PROGMEM para mover esses bytes para a memória FLASH
const uint8_t PS1_Startup[] PROGMEM = {
 
};

#endif // PS1_STARTUP_