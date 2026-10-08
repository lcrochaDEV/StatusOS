#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

class Display {
public:
    // Construtor recebendo a referência injetada do TFT único
    Display(TFT_eSPI& tft, uint8_t backlightPin = 21);
    ~Display();

    bool begin();
    void update();

    // Métodos de controle do Backlight
    void setBacklight(bool enable);
    void setBrightness(uint8_t val);
    void fadeTo(uint8_t targetValue, uint16_t durationMs = 300);

    uint8_t getBrightness() const { return _currentBrightness; }
    TFT_eSPI& getTft() { return _tft; }
    bool isInitialized() const { return disp != nullptr; }

private:
    TFT_eSPI& _tft; // Referência para a instância global do .ino
    uint8_t *draw_buf;
    lv_display_t *disp;
    uint32_t last_tick;

    uint8_t _backlightPin;
    uint8_t _currentBrightness;

    static const size_t DRAW_BUF_SIZE = (320 * 10 * sizeof(uint16_t));
    static void IRAM_ATTR my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};

#endif // DISPLAY_H