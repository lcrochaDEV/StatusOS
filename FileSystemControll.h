#ifndef FILE_SYSTEM_CONTROLL_H
#define FILE_SYSTEM_CONTROLL_H

#include <Arduino.h>
#include "ConsoleDependencies.h"

class FileSystemControll : public IBrightnessControl {
private:
    static const uint8_t PIN_BACKLIGHT = 21;    // Pino GPIO 21
    static const uint32_t PWM_FREQ = 5000;      // 5 kHz
    static const uint8_t PWM_RES = 8;           // 8 bits (0 a 255)
    static const uint8_t PWM_CHANNEL = 0;       // Canal PWM

    uint8_t brilho_atual_pct = 80;              // Valor padrão inicial (80%)
    uint8_t pwm_atual = 0;                      // Último valor PWM escrito no pino (0 a 255)
    bool iniciado = false;
    uint8_t brilho_antes_de_apagar = 80;      // Brilho (%) memorizado ao apagar, restaurado ao ligar
    bool display_apagado = false;             // true somente enquanto displayOff() mantém o backlight apagado

public:
    FileSystemControll();
    
    void begin();
    void setPinPwm(uint8_t valor); // 0 a 255
    void setPinPwm(bool ligado);   // LOW / HIGH

    // Novo método para abstração percentual (0 a 100%)
    void setBrilhoPorcentagem(uint8_t porcentagem) override {
        if (porcentagem > 100) porcentagem = 100;
        brilho_atual_pct = porcentagem;
        
        // Mapeia de 0-100% para 0-255 PWM
        uint8_t pwm_val = (porcentagem * 255) / 100;
        setPinPwm(pwm_val);
    }

    uint8_t obterBrilhoPorcentagem() const override {
        return brilho_atual_pct;
    }

    // Ajuste direto por PWM (0 a 255) mantendo a porcentagem sincronizada
    void setBrilhoPwm(uint8_t valor) {
        brilho_atual_pct = (uint8_t)(((uint16_t)valor * 100 + 127) / 255);
        setPinPwm(valor);
    }

    uint8_t obterPwm() const {
        return pwm_atual;
    }
        
    // Apaga o backlight memorizando o brilho atual
    void displayOff() override {
        if (brilho_atual_pct > 0) brilho_antes_de_apagar = brilho_atual_pct;
        setBrilhoPorcentagem(0);
        display_apagado = true;
    }

    void displayOn() override {
        if (!display_apagado) return;
        display_apagado = false;
        setBrilhoPorcentagem(brilho_antes_de_apagar);
    }
};

// Instância global para ser reutilizada no sistema
extern FileSystemControll g_file_system_ctrl;

#endif // FILE_SYSTEM_CONTROLL_H