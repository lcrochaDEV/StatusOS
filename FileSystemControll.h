#ifndef FILE_SYSTEM_CONTROLL_H
#define FILE_SYSTEM_CONTROLL_H

#include <Arduino.h>

class FileSystemControll {
private:
    static const uint8_t PIN_BACKLIGHT = 21; // Pino GPIO 21[cite: 6]
    static const uint32_t PWM_FREQ = 5000;    // 5 kHz[cite: 6]
    static const uint8_t PWM_RES = 8;         // 8 bits (0 a 255)[cite: 6]
    static const uint8_t PWM_CHANNEL = 0;     // Canal PWM[cite: 6]

    uint8_t brilho_atual_pct = 80;            // Valor padrão inicial (80%)

public:
    FileSystemControll();
    
    void begin();
    void setPinPwm(uint8_t valor); // 0 a 255[cite: 6]
    void setPinPwm(bool ligado);   // LOW / HIGH[cite: 6]

    // Novo método para abstração percentual (0 a 100%)
    void setBrilhoPorcentagem(uint8_t porcentagem) {
        if (porcentagem > 100) porcentagem = 100;
        brilho_atual_pct = porcentagem;
        
        // Mapeia de 0-100% para 0-255 PWM
        uint8_t pwm_val = (porcentagem * 255) / 100;
        setPinPwm(pwm_val);
    }

    uint8_t obterBrilhoPorcentagem() const {
        return brilho_atual_pct;
    }
};

// Instância global para ser reutilizada no sistema
extern FileSystemControll g_file_system_ctrl;

#endif // FILE_SYSTEM_CONTROLL_H