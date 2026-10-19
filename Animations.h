#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <functional>
#include "ConsoleDependencies.h"

class Animations: public IAnimationControl {
public:
    explicit Animations(TFT_eSPI& tft);
    ~Animations() override;

    void begin();
    void update(); 

    bool playAnimation(GifID animType) override;
    bool playGif(const uint8_t* gifData, size_t gifSize);
    void stop() override;

    void onAnimationEnd(std::function<void()> callback);

    // Condições (injetada) que encerram a tela de boot, ex.: WI-FI conectado. sem condições, o boot só termina via stop() ou playAnimation(GifID::NONE)
    void setBootExitCondition(std::function<bool()> conditions);

    bool isPlaying() const override { return _isPlaying || _isTestMode; }
    GifID getCurrentAnimation() const override { return _currentAnim; }

    //GIFs LIST
    void testRedScreen() override;
    void foraDoAr() override;
    //void ps1() override;

private:
    TFT_eSPI& _tft;
    
    AnimatedGIF* _gif = nullptr;
    bool _isPlaying = false;

    bool _isTestMode = false;
    bool _inStop = false; // Trava contra recursao infinita
    
    GifID _currentAnim = GifID::NONE;
    std::function<void()> _onEndCallback = nullptr;
    std::function<bool()> _bootExitCondition = nullptr;

    //Temporização não bloqueante
    uint32_t _nextGifFrameMs = 0;
    uint32_t _bootStartMs = 0;
    uint32_t _foraStartMs = 0;
    uint32_t _foraLastFrameMs = 0;
    int _foraFrame = 0;
    int _foraTotalFrames = 0;

    void atualizarForaDoAr();
    static void GIFDraw(GIFDRAW *pDraw);
};

#endif // ANIMATIONS_H