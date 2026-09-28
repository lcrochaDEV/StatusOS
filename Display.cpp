#include "Display.h"

Display::Display(uint8_t backlightPin) 
    : draw_buf(nullptr), disp(nullptr), last_tick(0), 
      _backlightPin(backlightPin), _currentBrightness(255) {}

Display::~Display() {
    if (draw_buf != nullptr) {
        heap_caps_free(draw_buf);
        draw_buf = nullptr;
    }
}

IRAM_ATTR void Display::my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    Display *self = static_cast<Display *>(lv_display_get_user_data(disp));
    if (!self) return;

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    self->tft.startWrite();
    self->tft.setAddrWindow(area->x1, area->y1, w, h);
    self->tft.pushColors((uint16_t *)px_map, w * h, true);
    self->tft.endWrite();

    lv_display_flush_ready(disp);
}

bool Display::begin() {
    if (disp != nullptr) return true; 

    // 1. Configuração do pino de PWM do Backlight (GPIO 21)
    pinMode(_backlightPin, OUTPUT);
    setBrightness(_currentBrightness); // Inicia em 100% (255)

    // 2. Inicialização do TFT
    tft.init();
    tft.setRotation(1);
    tft.invertDisplay(false); 

    // 3. Alocação do buffer para LVGL v9
    if (draw_buf == nullptr) {
        draw_buf = static_cast<uint8_t *>(heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        if (draw_buf == nullptr) {
            Serial.println("ERRO: Falha ao alocar memória Heap para o LVGL!");
            return false;
        }
    }

    // 4. Configuração do LVGL v9
    lv_init();

    disp = lv_display_create(320, 240);
    if (disp == nullptr) {
        Serial.println("ERRO: Falha ao criar o display do LVGL!");
        return false;
    }

    lv_display_set_user_data(disp, this);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    last_tick = millis();
    return true;
}

void Display::update() {
    if (disp == nullptr) return;

    uint32_t current_time = millis();
    lv_tick_inc(current_time - last_tick);
    last_tick = current_time;

    lv_timer_handler();
}

// Ligar/Desligar Rápido
void Display::setBacklight(bool enable) {
    setBrightness(enable ? 255 : 0);
}

// Define o valor exato de PWM no Pino 21 (0 a 255)
void Display::setBrightness(uint8_t val) {
    _currentBrightness = val;
    analogWrite(_backlightPin, _currentBrightness);
}

// Efeito Gradual: Clareia ou Escurece até o valor desejado
void Display::fadeTo(uint8_t targetValue, uint16_t durationMs) {
    if (targetValue == _currentBrightness) return;

    int steps = abs(targetValue - _currentBrightness);
    if (steps == 0) return;

    int stepDelay = durationMs / steps;
    int stepDirection = (targetValue > _currentBrightness) ? 1 : -1;

    while (_currentBrightness != targetValue) {
        _currentBrightness += stepDirection;
        analogWrite(_backlightPin, _currentBrightness);
        delay(stepDelay);
    }
}

/*EXEMPLO DE USO DO METODO CONTROLE DE BRILHO*/
/*
// Alterar o brilho para 50% (valor 128) de forma instantânea:
display.setBrightness(128);

// Clarear gradualmente para o máximo (255) em 500ms:
display.fadeTo(255, 500);

// Escurecer gradualmente até apagar (0) em 1 segundo:
display.fadeTo(0, 1000);
*/