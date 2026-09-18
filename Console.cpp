#include "Console.h"

Console::Console(const char* consoleText) 
  : _consoleText(consoleText) 
{}

void Console::helloWord(const char* consoleText) {
  // Se o argumento 'text' for nulo, usamos o 'consoleText' da classe
  const char* msg = (consoleText != nullptr) ? consoleText : _consoleText;
  
  if (_consoleText) return menssageViewMsg(_consoleText);
  if (msg != nullptr) menssageViewMsg(msg);
  
}

void Console::menssageViewMsg(const char* consoleText) {
  Serial.println(consoleText);
}

void Console::consoleView() {
  if (Serial.available() > 0) {
    // readStringUntil evita atrasos de timeout do readString comum
    String consoleText = Serial.readStringUntil('\n');
    consoleText.trim();
    
    if (consoleText.length() > 0) {
      consoleText.toUpperCase();
      commands_envio(String(consoleText.c_str()));
    }
  }
}

void Console::commands_envio(const String& command) {
  String prefixo = "Mochi> ";
  menssageViewMsg((prefixo + command).c_str());

  if (command == "HELP") menssageViewMsg("Comandos: SHOWDATA, HELP");
  
  else menssageViewMsg("Comando inexistente. Digite HELP.");
  menssageViewMsg(_consoleText);
}