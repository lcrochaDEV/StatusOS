#include "Animations.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif
#include "ciclope.h"
#include "foradoar.h"

static TFT_eSPI* pTftInstance = nullptr;

static constexpr uint32_t MIN_FREE_HEAP_GIF_BYTES = 20000;
static constexpr uint32_t FORA_DO_AR_FRAME_MS = 50;
static constexpr uint32_t BOOT_TIMEOUT_MS = 15000; 

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

void Animations::begin() {
    pTftInstance = &_tft;

    // Condição padrão para encerrar a tela de boot: Wi-Fi conectado (substituível por setBootExitCondition)
    if (!_bootExitCondition) _bootExitCondition = []() { return WiFi.status() == WL_CONNECTED; };
}


void Animations::onAnimationEnd(std::function<void()> callback) {
    _onEndCallback = callback;
}

void Animations::setBootExitCondition(std::function<bool()> condition) {
    _bootExitCondition = condition;
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
            if (_currentAnim == GifID::BOOT) {
                // Se já estiver exibindo o boot, a segunda chamada desliga e volta ao dashboard
                stop();
                _isTestMode = false;
            } else {
                
                // Primeira chamada: exibe a imagem de boot e trava o modo de teste
                stop();
                _currentAnim = GifID::BOOT;
                _isTestMode = true; 
                _bootStartMs = millis();
                
                _tft.startWrite();
                // Processa linha por linha (320 pixels por vez) para corrigir as cores
                for (int y = 0; y < 240; y++) {
                    uint16_t linhaBuffer[320];
                    for (int x = 0; x < 320; x++) {
                        int index = y * 320 + x;
                        linhaBuffer[x] = __builtin_bswap16(((uint16_t*)ciclope)[index]);
                    }
                    _tft.pushImage(0, y, 320, 1, linhaBuffer);
                }
                _tft.endWrite();
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            return true;
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
        _nextGifFrameMs = 0;
        return true;
    }

    _currentAnim = GifID::NONE;
    Serial.println("ERRO: Falha ao abrir o arquivo GIF!");
    return false;
}

void Animations::update() {
    if (_isTestMode) {
        // Exemplo: se estiver no modo boot, podemos checar a internet aqui de forma não bloqueante
        if (_currentAnim == GifID::BOOT) {
            // Supondo que você tenha acesso ao status do Wi-Fi aqui ou uma flag:
            if (_bootExitCondition && _bootExitCondition()) {
                Serial.println("[Boot] Internet conectada! Encerrando tela de boot...");
                stop(); // Isso vai parar o boot, limpar o estado e disparar o callback para o dashboard
            } else if (millis() - _bootStartMs >= BOOT_TIMEOUT_MS) {
                Serial.println("[Boot] Tempo limite da tela de boot. Seguindo para o Dashboard...");
                stop();
            }
        } else if (_currentAnim == GifID::FORA_DO_AR) {
            atualizarForaDoAr();
        }
        
        yield(); // Libera ciclos de CPU para o servidor Web HTTP
        return; 
    }

    if (!_isPlaying || !_gif) return;

    // Respeita o atraso do quadro anterior sem bloquear o loop
    if (static_cast<int32_t>(millis() - _nextGifFrameMs) < 0) {
        yield();
        return;
    }

    int delayMs = 0;
    if (!_gif->playFrame(false, &delayMs)) {
        stop();
    } else {
        _nextGifFrameMs = millis() + (delayMs > 0 ? (uint32_t)delayMs : 0U);
        yield();
    }
}


void Animations::stop() {
    if (_inStop) return;
    _inStop = true;

    _isTestMode = false;      
    if (_gif && _isPlaying) _gif->close();

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

void Animations::foraDoAr() {
    // 1. Inicialização dos estados e flags da classe (Igual ao seu padrão)
    stop();
    _currentAnim = GifID::FORA_DO_AR;
    _isTestMode = true; 

    // Descobre dinamicamente o total de quadros no seu arquivo .h
    _foraTotalFrames = sizeof(tft_image_frames) / sizeof(tft_image_frames[0]);

    Serial.println("[Animations] Iniciando loop Fora do Ar...");

    // 2. REGISTRO DO TEMPO INICIAL
    _foraStartMs = millis();
    _foraLastFrameMs = _foraStartMs - FORA_DO_AR_FRAME_MS; // Força o desenho imediato do primeiro quadro
    _foraFrame = 0; // Índice do frame atual
}

// Avança a animação "Fora do Ar" de forma não bloqueante (chamado por update())
void Animations::atualizarForaDoAr() {
    uint32_t agora = millis();

    // Micro-espera interna de 50ms para controlar o FPS de forma não-bloqueante ao chip
    if (agora - _foraLastFrameMs < FORA_DO_AR_FRAME_MS) return;
    _foraLastFrameMs = agora;

    // Desenha o frame atual na tela (320x240 pixels)
    // O swap de bytes é restaurado para não afetar as demais telas (ex.: BOOT)
    bool swapAnterior = _tft.getSwapBytes();
    _tft.setSwapBytes(true); 
    _tft.pushImage(0, 0, 320, 240, tft_image_frames[_foraFrame]);
    _tft.setSwapBytes(swapAnterior);

    // Avança cíclica e infinitamente para o próximo quadro da animação
    _foraFrame = (_foraFrame + 1) % _foraTotalFrames;

    yield(); // Alimenta o Watchdog do ESP32 e mantém as conexões de rede ativas
}