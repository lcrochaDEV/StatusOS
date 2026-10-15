#ifndef CONSOLE_DEPENDECIES_H
#define CONSOLE_DEPENDECIES_H


#include <Arduino.h>

enum class GifID {
    NONE,
    BOOT,       
    NOT_WIFI,   
    NOT_WORKER, 
    BUG_FRAME,  
    STANDBY,
    FORA_DO_AR     
};

// Interfaces mínimas consumidas pelo Console (inversão de dependência).
class IAnimationControl {
    public:
        virtual ~IAnimationControl() = default;
        virtual bool playAnimation(GifID animationType) = 0;
        virtual void stop() = 0;
        virtual void testRedScreen() = 0;
        virtual void foraDoAr() = 0;
        virtual bool isPlaying() const = 0;
        virtual GifID getCurrentAnimation() const = 0;
};

class IBrightnessControl {
    public:
        virtual ~IBrightnessControl() = default;
        virtual void setBrilhoPorcentagem(uint8_t porcentagem) = 0;
        virtual uint8_t obterBrilhoPorcentagem() const = 0;
};

class IPopupControl {
    public:
        virtual ~IPopupControl() = default;
        virtual void abrirWifi() = 0;
        virtual void fecharWifi() = 0;
        virtual bool wifiAberto() const = 0;
        virtual void abrirBrilho() = 0;
        virtual void fecharBrilho() = 0;
        virtual bool brilhoAberto() const = 0;
};

#endif // CONSOLE_DEPENDECIES_H