#include "Animations.h"

// Ponteiro estático para a instância do TFT (necessário para a callback estática GIFDraw)
static TFT_eSPI* pTftInstance = nullptr;

Animations::Animations(TFT_eSPI& tft, uint8_t backlightPin)
    : _tft(tft), _backlightPin(backlightPin), _isActive(true), _isPlaying(false) {}

void Animations::begin() {
    pinMode(_backlightPin, OUTPUT);
    digitalWrite(_backlightPin, HIGH);
    
    pTftInstance = &_tft; // Define a referência global do TFT
    _gif.begin(GIF_PALETTE_RGB565_BE); // Configura a paleta de cores para o formato Big-Endian do TFT
}

bool Animations::playGif(const uint8_t* gifData, size_t gifSize) {
    if (!_isActive) return false;
    
    if (_isPlaying) {
        _gif.close();
        _isPlaying = false;
    }

    // Abre o GIF diretamente da memória Flash
    if (_gif.open((uint8_t*)gifData, gifSize, GIFDraw)) {
        _gif.setDrawType(GIF_DRAW_COOKED); // Converte a paleta de cores diretamente para RGB565
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
    // Decodifica e desenha o quadro atual não-bloqueante
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

// Callback estática acionada para cada linha de pixels decodificada do GIF
void Animations::GIFDraw(GIFDRAW *pDraw) {
    if (!pTftInstance) return;
    if (pDraw->y >= pDraw->iHeight) return;

    uint16_t y = pDraw->iY + pDraw->y;
    uint16_t x = pDraw->iX;
    uint16_t w = pDraw->iWidth;
    uint16_t *pPixels = (uint16_t *)pDraw->pPixels;

    pTftInstance->startWrite();

    if (pDraw->ucHasTransparency) {
        uint8_t ucTransparent = pDraw->ucTransparent;
        int iCount = 0;
        int iStart = 0;

        // Otimização: Agrupa pixels visíveis em blocos continuos antes de enviar ao TFT
        for (int i = 0; i < w; i++) {
            if (pDraw->pPixels[i] == ucTransparent) {
                if (iCount > 0) {
                    pTftInstance->setAddrWindow(x + iStart, y, iCount, 1);
                    pTftInstance->pushPixels(&pPixels[iStart], iCount);
                    iCount = 0;
                }
            } else {
                if (iCount == 0) {
                    iStart = i;
                }
                iCount++;
            }
        }
        if (iCount > 0) {
            pTftInstance->setAddrWindow(x + iStart, y, iCount, 1);
            pTftInstance->pushPixels(&pPixels[iStart], iCount);
        }
    } else {
        // Envia a linha inteira de uma vez diretamente para a tela
        pTftInstance->setAddrWindow(x, y, w, 1);
        pTftInstance->pushPixels(pPixels, w);
    }

    pTftInstance->endWrite();
}