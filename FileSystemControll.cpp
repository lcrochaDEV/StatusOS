#include "FileSystemControll.h"

FileSystemControll::FileSystemControll() {}

void FileSystemControll::begin() {
    // Configura o periférico PWM do ESP32 com suporte às versões do Core Arduino
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttachChannel(PIN_BACKLIGHT, PWM_FREQ, PWM_RES, PWM_CHANNEL);
#else
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_BACKLIGHT, PWM_CHANNEL);
#endif

    // Inicializa o pino como saída com estado inicial desligado
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, LOW);
}

void FileSystemControll::setPinPwm(uint8_t valor) {
    if (valor == 0) {
        // Estado digital BAIXO (Desligado)
        digitalWrite(PIN_BACKLIGHT, LOW);
    } 
    else if (valor == 255) {
        // Estado digital ALTO (100% de brilho/potência)
        digitalWrite(PIN_BACKLIGHT, HIGH);
    } 
    else {
        // Modulação PWM para valores intermediários entre 1 e 254
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(PIN_BACKLIGHT, valor);
#else
        ledcWrite(PWM_CHANNEL, valor);
#endif
    }
}

void FileSystemControll::setPinPwm(bool ligado) {
    // Acionamento em modo discreto HIGH (255) ou LOW (0)
    setPinPwm(ligado ? (uint8_t)255 : (uint8_t)0);
}