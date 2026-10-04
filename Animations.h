#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>

class Animations {
public:
    // Construtor recebendo a referência da instância do TFT e o pino de Backlight
    Animations(TFT_eSPI& tft, uint8_t backlightPin);
    ~Animations() = default;

    void begin();
    void update(); // Processa a atualização de quadros no loop principal
    void stop();

    // Métodos para carregar e reproduzir GIFs a partir da memória Flash (PROGMEM)
    bool playGif(const uint8_t* gifData, size_t gifSize);
    void notwifi(const uint8_t* gifData, size_t size);
    void bugframe(const uint8_t* gifData, size_t size);

    // Controle de estado e energia
    void controlPower(bool enable);
    bool isPlaying() const { return _isPlaying; }
    bool isActive() const { return _isActive; }

private:
    TFT_eSPI& _tft;
    AnimatedGIF _gif;
    uint8_t _backlightPin;

    bool _isActive;
    bool _isPlaying;

    // Callback estática acionada pelo AnimatedGIF para renderizar linhas de pixels no display
    static void GIFDraw(GIFDRAW *pDraw);
};

#endif // ANIMATIONS_H