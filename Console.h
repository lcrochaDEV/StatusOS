#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <stdarg.h>
#include <functional>
#include "ConsoleDependencies.h"
#include "FileSystemControll.h"
#include "ui_dashboard.h"
#include "Hours_Time.h"
#include "Logger.h"

class Console {
  public:
    Console(const char* consoleText = "Mochi> ");

    // Método para conectar a instância do Animations ao Console
    void setAnimations(IAnimationControl* animPtr) { _anim = animPtr; }
    void setBrightnessControl(IBrightnessControl* brightnessPtr) { _brightness = brightnessPtr; }
    void setIPopupControl(IPopupControl* popupPtr) { _popup = popupPtr; }

    // Métodos legados/compatibilidade
    void helloWord(const char* consoleText = nullptr);
    void menssageViewMsg(const char* consoleText = nullptr);
    void consoleView();

    // Sistema de Log Unificado (Proxies para Logger)
    void log(const char* message) { Logger::instance().log(message); }
    void log(const String& message) { Logger::instance().log(message); }
    void logf(const char* format, ...);
    void setLogState(bool enable) { Logger::instance().setEnabled(enable); }
    bool isLogEnabled() const { return Logger::instance().isEnabled(); }

  private:
    const char* _consoleText;

    //POPUPS E ANIMAÇÕES VARIAVEIS DE ESTADO
    IAnimationControl* _anim = nullptr;
    IBrightnessControl* _brightness = nullptr;
    IPopupControl* _popup = nullptr;

    typedef void (Console::*CommandHandler)(const String& param);
    struct CommandEntry {
        const char* name;
        CommandHandler handler;
    };

    void commands_envio(const String& command);
    void printPrompt();

    void cmdHelp(const String& param);
    void cmdWifiPopup(const String& param);
    void cmdBrightPopup(const String& param);
    void cmdBright(const String& param);
    void cmdSetBrightness(const String& param);
    void cmdDisplayOn(const String& param);
    void cmdDisplayOff(const String& param);
    void cmdLogo(const String& param);
    void cmdAnimacao(const String& param);
    void cmdTestRed(const String& param);
    void cmdStop(const String& param);
    void cmdForaDoAr(const String& param);
    void cmdPs1(const String& param);
    void cmdOlho(const String& param);

    void cmdLogOn(const String& param);
    void cmdLogOff(const String& param);
    void cmdWake(const String& param);
    void cmdScreenWol(const String& param);
    void cmdScreenMain(const String& param);
    void cmdScreenCrypto(const String& param);

    virtual void cmdClear(const String& param);
    bool animationReady();

    void limparTerminal();
};

extern Console console;

#endif
