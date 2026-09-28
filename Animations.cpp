#include "Animations.h"

// Ponteiro estático para a instância do TFT
static TFT_eSPI* pTftInstance = nullptr;

Animations::Animations(TFT_eSPI& tft, uint8_t backlightPin)
    : _tft(tft), _backlightPin(backlightPin), _isActive(true), _isPlaying(false) {}

void Animations::begin() {
    pinMode(_backlightPin, OUTPUT);
    digitalWrite(_backlightPin, HIGH);
    
    pTftInstance = &_tft; // Define a referência do TFT na inicialização
    _gif.begin(GIF_PALETTE_RGB565_BE); // Configura paleta no formato RGB565 do TFT
}

bool Animations::playGif(const uint8_t* gifData, size_t gifSize) {
    if (!_isActive) return false;
    
    if (_isPlaying) {
        _gif.close();
        _isPlaying = false;
    }

    // Abre o GIF diretamente da Flash sem alocar buffer extra de quadro na RAM
    if (_gif.open((uint8_t*)gifData, gifSize, GIFDraw)) {
        _gif.setDrawType(GIF_DRAW_COOKED); // Converte a paleta para cores RGB565
        _isPlaying = true;
        Serial.printf("GIF carregado: %dx%d\n", _gif.getCanvasWidth(), _gif.getCanvasHeight());
        return true;
    }
    
    Serial.println("ERRO: Falha ao abrir a imagem GIF.");
    return false;
}

void Animations::update() {
    if (!_isActive || !_isPlaying) return;

    int delayMs = 0;
    // Decodifica e desenha o quadro atual
    if (!_gif.playFrame(false, &delayMs)) {
        _gif.reset(); // Reinicia a animação ao chegar ao final
    }
}

void Animations::stop() {
    if (_isPlaying) {
        _gif.close();
        _isPlaying = false;
    }
}

void Animations::controlPower(bool enable) {
    if (_isActive == enable) return;
    
    _isActive = enable;
    digitalWrite(_backlightPin, enable ? HIGH : LOW);
    
    if (!enable) {
        stop();
        _tft.fillScreen(TFT_BLACK);
    }
}

void Animations::notwifi(const uint8_t* gifData, size_t size) { playGif(gifData, size); }
void Animations::bugframe(const uint8_t* gifData, size_t size) { playGif(gifData, size); }

// Callback acionada para cada linha decodificada do GIF
void Animations::GIFDraw(GIFDRAW *pDraw) {
    if (!pTftInstance) return;
    if (pDraw->y >= pDraw->iHeight) return;

    uint16_t y = pDraw->iY + pDraw->y;
    uint16_t x = pDraw->iX;
    uint16_t w = pDraw->iWidth;

    // Habilita a comunicação com o display
    pTftInstance->startWrite();

    if (pDraw->ucHasTransparency) {
        uint8_t ucTransparent = pDraw->ucTransparent;
        uint16_t *pPixels = (uint16_t *)pDraw->pPixels;
        
        for (int i = 0; i < w; i++) {
            if (pDraw->pPixels[i] != ucTransparent) {
                pTftInstance->drawPixel(x + i, y, pPixels[i]);
            }
        }
    } else {
        // Envia a linha inteira diretamente para o TFT
        pTftInstance->setAddrWindow(x, y, w, 1);
        pTftInstance->pushPixels((uint16_t*)pDraw->pPixels, w);
    }

    // Finaliza a transação SPI
    pTftInstance->endWrite();
}