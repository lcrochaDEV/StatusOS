#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <functional>

enum class GifID {
    NONE,
    BOOT,       
    NOT_WIFI,   
    NOT_WORKER, 
    BUG_FRAME,  
    STANDBY,
    FORA_DO_AR     
};

class Animations {
public:
    explicit Animations(TFT_eSPI& tft);
    ~Animations();

    void begin();
    void update(); 

    bool playAnimation(GifID animType);
    bool playGif(const uint8_t* gifData, size_t gifSize);
    void stop();

    void onAnimationEnd(std::function<void()> callback);

    bool isPlaying() const { return _isPlaying || _isTestMode; }
    bool isActive() const { return _currentAnim != GifID::NONE; }
    GifID getCurrentAnimation() const { return _currentAnim; }

    //GIFs LIST
    void testRedScreen();
    void foraDoAr();

private:
    TFT_eSPI& _tft;
    AnimatedGIF* _gif = nullptr;
    bool _isPlaying = false;
    bool _isTestMode = false;
    bool _inStop = false; // Trava contra recursao infinita
    
    // Flag de controle local simples para gerenciar o estado da animação
    volatile bool _rodarForaDoAr = false;

    GifID _currentAnim = GifID::NONE;
    std::function<void()> _onEndCallback = nullptr;

    static void GIFDraw(GIFDRAW *pDraw);
    static void animationTask(void* pvParameters); // Task interna
    TaskHandle_t _taskHandle = nullptr;
};

#endif // ANIMATIONS_H