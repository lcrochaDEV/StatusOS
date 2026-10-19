#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <stdarg.h>
#include <functional>
#include "ConsoleDependencies.h"
#include "FileSystemControll.h"
#include "ui_dashboard.h"
#include "Hours_Time.h"

class Console {
  public:
    Console(const char* consoleText = "Mochi> ");
    
    // Método para conectar a instância do Animations ao Console
    void setAnimations(IAnimationControl* animPtr) { _anim = animPtr; }
    void setBrightnessControl(IBrightnessControl* brightnessPtr) { _brightness = brightnessPtr; }
    void setIPopupControl(IPopupControl* popupPtr) { _popup = popupPtr; }
    //void setWakeAction(std::function<void()> action) { _wakeAction = action; }


    // Métodos legados/compatibilidade
    void helloWord(const char* consoleText = nullptr);
    void menssageViewMsg(const char* consoleText = nullptr);
    void consoleView();

    // Sistema de Log Unificado
    void log(const char* message);
    void log(const String& message);
    void logf(const char* format, ...);
    void setLogState(bool enable);
    bool isLogEnabled() const;

  private:
    const char* _consoleText;
    bool _logsEnabled;
    //POPUPS E ANIMAÇÕES VARIAVEIS DE ESTADO
    IAnimationControl* _anim = nullptr; // Ponteiro interno para controle de animação
    IBrightnessControl* _brightness = nullptr; // Ponteiro para controle de brilho
    IPopupControl* _popup = nullptr; // Ponteiro para controle de popups
    //std::function<void()> _wakeAction = nullptr;


    // Tabela de comandos (nome em maíusculas -> manipulador)
    typedef void (Console::*CommandHandler)(const String& param);
    struct CommandEntry {
        const char* name;
        CommandHandler handler;
    };

    void commands_envio(const String& command);
    void printPrompt();

    // Manipuladores de comandos
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

    void cmdLogOn(const String& param);   //ATIVA OS LOGS CONTINUOS
    void cmdLogOff(const String& param);  //DESATIVA OS LOGS CONTINUOS
    void cmdWake(const String& param);
    
    virtual void cmdClear(const String& param);
    bool animationReady();
    
    void limparTerminal();
};

extern Console console;

#endif