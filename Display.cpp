#include "Display.h"

// Construtor inicializa a referência _tft
Display::Display(TFT_eSPI& tft, uint8_t backlightPin) 
    : _tft(tft), draw_buf(nullptr), disp(nullptr), last_tick(0), 
      _backlightPin(backlightPin), _currentBrightness(255) {}

Display::~Display() {
    if (draw_buf != nullptr) {
        heap_caps_free(draw_buf);
        draw_buf = nullptr;
    }
}

void Display::my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    Display *self = static_cast<Display *>(lv_display_get_user_data(disp));
    if (!self) return;

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // Utiliza a referência injetada do hardware compartilhado
    self->_tft.startWrite();
    self->_tft.setAddrWindow(area->x1, area->y1, w, h);
    self->_tft.pushColors((uint16_t *)px_map, w * h, true);
    self->_tft.endWrite();

    lv_display_flush_ready(disp);
}

bool Display::begin() {
    if (disp != nullptr) return true; 

    // 1. Configuração obrigatória do pino de hardware (Mantido conforme solicitado)
    pinMode(_backlightPin, OUTPUT);

    // CONTROLE DE FLUXO PRIORITÁRIO:
    // Pega o valor real de 80% configurado e autorizado na classe FileSystemControll
    uint8_t brilhoInicial = g_file_system_ctrl.obterBrilhoPorcentagem();
    
    // Mapeia de porcentagem (0-100) para PWM de 8 bits (0-255) para o Display obedecer
    uint8_t pwmMapeado = (brilhoInicial * 255) / 100;
    
    // Aplica o valor de 80% convertido no display ao iniciar ou reiniciar
    setBrightness(pwmMapeado);

    // Inicialização correta usando o membro _tft da classe Display
    _tft.init();
    _tft.setRotation(1);           // Ajuste para paisagem (0, 1, 2 ou 3)
    _tft.invertDisplay(false);     // Experimente alterar para 'true' se estiver tudo cinza/branco
    _tft.fillScreen(TFT_BLACK);    // Limpa a tela com preto para testar se o display responde
    
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
    _currentBrightness = g_file_system_ctrl.obterPwm(); // O brilho pode ter sido alterado pela UI/Console
    if (targetValue == _currentBrightness) return;

    int steps = abs(targetValue - _currentBrightness);
    if (steps == 0) return;

    int stepDelay = durationMs / steps;
    if (stepDelay < 1) stepDelay = 1;
    int stepDirection = (targetValue > _currentBrightness) ? 1 : -1;

    while (_currentBrightness != targetValue) {
        setBrightness(static_cast<uint8_t>(_currentBrightness + stepDirection));
        delay(stepDelay);
    }
}