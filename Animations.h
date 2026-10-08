#ifndef ANIMATIONS_H
#define ANIMATIONS_H

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
    STANDBY     
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

    void testRedScreen();

private:
    TFT_eSPI& _tft;
    AnimatedGIF* _gif = nullptr;
    bool _isPlaying = false;
    bool _isTestMode = false;
    bool _inStop = false; // Trava contra recursao infinita
    GifID _currentAnim = GifID::NONE;
    std::function<void()> _onEndCallback = nullptr;

    static void GIFDraw(GIFDRAW *pDraw);
    static void animationTask(void* pvParameters); // Task interna
    TaskHandle_t _taskHandle = nullptr;
};

#endif // ANIMATIONS_H