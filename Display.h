#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

class Display {
public:
    // Construtor definindo o GPIO 21 como pino padrão do Backlight
    Display(uint8_t backlightPin = 21);
    ~Display();

    bool begin();
    void update();

    // --- Métodos de Controle do Backlight / PWM (Pino 21) ---
    void setBacklight(bool enable);                          // Liga (255) ou Desliga (0)
    void setBrightness(uint8_t val);                        // Ajusta o brilho diretamente (0 a 255)
    void fadeTo(uint8_t targetValue, uint16_t durationMs = 300); // Transição suave para escurecer ou clarear

    uint8_t getBrightness() const { return _currentBrightness; }
    TFT_eSPI& getTft() { return tft; }
    bool isInitialized() const { return disp != nullptr; }

private:
    TFT_eSPI tft;
    uint8_t *draw_buf;
    lv_display_t *disp;
    uint32_t last_tick;

    // Controle de hardware do Backlight
    uint8_t _backlightPin;
    uint8_t _currentBrightness;

    static const size_t DRAW_BUF_SIZE = (320 * 10 * sizeof(uint16_t));
    static void IRAM_ATTR my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};

#endif // DISPLAY_H