#include "Animations.h"
#include "PS1_Startup.h"

static TFT_eSPI* pTftInstance = nullptr;

Animations::Animations(TFT_eSPI& tft)
    : _tft(tft), _gif(nullptr), _isPlaying(false), _isTestMode(false), 
      _inStop(false), _currentAnim(GifID::NONE), _onEndCallback(nullptr) {}

Animations::~Animations() {
    stop();
    if (_gif) {
        delete _gif;
        _gif = nullptr;
    }
}

void Animations::begin() {  pTftInstance = &_tft;   }

void Animations::onAnimationEnd(std::function<void()> callback) {
    _onEndCallback = callback;
}

bool Animations::playAnimation(GifID animType) {
    if (animType == GifID::NONE) {
        stop();
        return true;
    }

    if (_isPlaying && _currentAnim == animType) return true;

    const uint8_t* gifData = nullptr;
    size_t gifSize = 0;

    switch (animType) {
        case GifID::BOOT:
            gifData = PS1_Startup;
            gifSize = sizeof(PS1_Startup);
            break;

        case GifID::NOT_WIFI:
            // gifData = not_wifi_gif;
            // gifSize = sizeof(not_wifi_gif);
            break;

        case GifID::NOT_WORKER:
            // gifData = not_worker_gif;
            // gifSize = sizeof(not_worker_gif);
            break;

        case GifID::BUG_FRAME:
            // gifData = bug_frame_gif;
            // gifSize = sizeof(bug_frame_gif);
            break;

        case GifID::STANDBY:
            // gifData = standby_gif;
            // gifSize = sizeof(standby_gif);
            break;

        default:
            Serial.println("ERRO: Tipo de animacao nao mapeado!");
            return false;
    }

    if (gifData && gifSize > 0) {
        _currentAnim = animType;
        return playGif(gifData, gifSize);
    }

    return false;
}

bool Animations::playGif(const uint8_t* gifData, size_t gifSize) {
    if (_isPlaying) stop();

    pTftInstance = &_tft;

    // Alocação sob demanda (Lazy Loading)
    if (!_gif) {
        // Verifica se há memória suficiente antes de alocar
        if (ESP.getFreeHeap() < 20000) { 
            Serial.println("[ERRO] Memória Heap insuficiente para carregar o GIF!");
            return false;
        }

        _gif = new (std::nothrow) AnimatedGIF();
        if (!_gif) {
            Serial.println("[ERRO] Falha ao alocar memória para o AnimatedGIF!");
            return false;
        }
        _gif->begin(GIF_PALETTE_RGB565_BE);
    }

    if (_gif->open((uint8_t*)gifData, gifSize, GIFDraw)) {
        _gif->setDrawType(GIF_DRAW_COOKED);
        _isPlaying = true;
        return true;
    }

    _currentAnim = GifID::NONE;
    Serial.println("ERRO: Falha ao abrir o arquivo GIF!");
    return false;
}

void Animations::update() {
    if (_isTestMode) {
        yield(); // Libera ciclos de CPU para o servidor Web HTTP no modo de teste
        return; 
    }

    if (!_isPlaying || !_gif) return;

    int delayMs = 0;
    if (!_gif->playFrame(false, &delayMs)) {
        stop(); // Encerra ao terminar o GIF e dispara o callback de forma segura
    } else {
        yield(); // Cede tempo para o ESPAsyncWebServer/AsyncTCP processar requisicoes
    }
}

void Animations::stop() {
    if (_inStop) return;
    _inStop = true;

    _isTestMode = false;      
    if (_gif && _isPlaying) {
        _gif->close();
    }

    _isPlaying = false;
    _currentAnim = GifID::NONE;
    
    // Libera a memória RAM assim que a animação termina
    if (_gif) {
        delete _gif;
        _gif = nullptr;
    }

    if (_onEndCallback) _onEndCallback();

    _inStop = false;
}

void Animations::GIFDraw(GIFDRAW *pDraw) {
    if (!pTftInstance) return;

    if (pDraw->y == 0) pTftInstance->setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
    
    pTftInstance->pushPixels((uint16_t*)pDraw->pPixels, pDraw->iWidth);
}

void Animations::testRedScreen() {
    stop();                   
    _isTestMode = true;       
    _tft.fillScreen(TFT_RED); 
    Serial.println("[Animations] Tela vermelha de teste ativada!");
}