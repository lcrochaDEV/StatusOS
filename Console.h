#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>
#include <stdarg.h>
#include "WifiConnect.h"
#include "FileSystemControll.h"
#include "ui_dashboard.h" 
#include "Animations.h" // Inclusão do gerenciador de Animações

class Console: public WifiConnect {
  public:
    Console(const char* consoleText = "Mochi> ");
    
    // Método para conectar a instância do Animations ao Console
    void setAnimations(Animations* animPtr) { _anim = animPtr; }

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
    bool _event_brilho = false;
    bool _event_popup = false;
    bool _redteste_event = false;
    Animations* _anim = nullptr; // Ponteiro interno para controle de animação
    
    void commands_envio(const String& command);
    void printPrompt();
};

extern Console console;

#endif