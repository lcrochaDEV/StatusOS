#ifndef FILE_SYSTEM_CONTROLL_H
#define FILE_SYSTEM_CONTROLL_H

#include <Arduino.h>

class FileSystemControll {
private:
    static const uint8_t PIN_BACKLIGHT = 21; // Pino GPIO 21 utilizado para o controle
    static const uint32_t PWM_FREQ = 5000;    // Frequência de 5 kHz para evitar oscilações
    static const uint8_t PWM_RES = 8;         // Resolução de 8 bits (0 a 255)
    static const uint8_t PWM_CHANNEL = 0;     // Canal PWM do ESP32

public:
    FileSystemControll();
    
    // Inicializa o pino e o periférico PWM no ESP32
    void begin();

    // Define o brilho/potência aceitando valores de 0 a 255
    // 0 = Desligado (LOW), 255 = Potência Máxima (HIGH), 1-254 = Ajuste PWM
    void setPinPwm(uint8_t valor);

    // Sobrecarga para acionamento direto em modo LIGADO (HIGH) ou DESLIGADO (LOW)
    void setPinPwm(bool ligado);
};

#endif