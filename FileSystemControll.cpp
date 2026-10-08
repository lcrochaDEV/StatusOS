#include "FileSystemControll.h"

FileSystemControll g_file_system_ctrl;

FileSystemControll::FileSystemControll() {}

void FileSystemControll::begin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttachChannel(PIN_BACKLIGHT, PWM_FREQ, PWM_RES, PWM_CHANNEL); 
#else
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES); 
    ledcAttachPin(PIN_BACKLIGHT, PWM_CHANNEL); 
#endif

    pinMode(PIN_BACKLIGHT, OUTPUT); 
    setBrilhoPorcentagem(brilho_atual_pct); // Liga com o brilho padrão inicial
}

void FileSystemControll::setPinPwm(uint8_t valor) {
    if (valor == 0) digitalWrite(PIN_BACKLIGHT, LOW); 
    else if (valor == 255) digitalWrite(PIN_BACKLIGHT, HIGH); 
    else {
        #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
                ledcWrite(PIN_BACKLIGHT, valor); 
        #else
                ledcWrite(PWM_CHANNEL, valor); 
        #endif
    }
}

void FileSystemControll::setPinPwm(bool ligado) {
    setPinPwm(ligado ? (uint8_t)255 : (uint8_t)0); 
}