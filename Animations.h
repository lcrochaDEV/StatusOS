#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>

class Animations {
public:
    Animations(TFT_eSPI& tft, uint8_t backlightPin = 21);
    
    void begin();
    void update(); // Executado no loop principal
    
    bool playGif(const uint8_t* gifData, size_t gifSize);
    void stop();
    void controlPower(bool enable);
    bool isPlaying() const { return _isPlaying; }

    // Atalhos
    void notwifi(const uint8_t* gifData, size_t size);
    void bugframe(const uint8_t* gifData, size_t size);

private:
    TFT_eSPI& _tft;
    AnimatedGIF _gif;
    uint8_t _backlightPin;
    bool _isActive;
    bool _isPlaying;

    static void GIFDraw(GIFDRAW *pDraw);
};