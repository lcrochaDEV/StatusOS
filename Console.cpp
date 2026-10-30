#include "Console.h"

// Utiliza variável estática para rastrear se é o primeiro log após o prompt
static bool firstLogAfterPrompt = true;

// Limite do buffer de entrada serial (evita crescimento ilimitado da String)
static const size_t MAX_INPUT_LENGTH = 64;

// Retorna true se a String contém apenas dígitos
static bool isDigitsOnly(const String& text) {
  if (text.length() == 0) return false;
  for (size_t i = 0; i < text.length(); i++) {
    if (!isdigit((unsigned char)text[i])) return false;
  }
  return true;
}

Console::Console(const char* consoleText)
  :
    _consoleText(consoleText != nullptr ? consoleText : "Mochi> "),
    _logsEnabled(false),
    // Implementações padrão (podem ser trocadas pelos setters); assim o .ino não precisa ligá-las
    _brightness(&g_file_system_ctrl),
    _popup(&g_popup_control)
{}

void Console::helloWord(const char* consoleText) {
  const char* msg = (consoleText != nullptr) ? consoleText : _consoleText;
  if (msg != nullptr) menssageViewMsg(msg);
}

void Console::menssageViewMsg(const char* consoleText) {
  if (consoleText != nullptr) Serial.println(consoleText);
}

// Imprime o prompt sem criar novas linhas
void Console::printPrompt() {
  if (_consoleText != nullptr) Serial.print(_consoleText);
  firstLogAfterPrompt = true; // O próximo log deve pular a linha do prompt
}

// Habilita ou desabilita os logs sem disparar printPrompt extra
void Console::setLogState(bool enable) {
  _logsEnabled = enable;
  String status = _logsEnabled ? "[SYSTEM] Logs Habilitados." : "[SYSTEM] Logs Deshabilitados.";
  menssageViewMsg(status.c_str());
  // Removido o printPrompt() daqui para evitar duplicar Mochi>
}

bool Console::isLogEnabled() const {
  return _logsEnabled;
}

// Método central para canalizar logs de todas as classes
void Console::log(const char* message) {
  if (!_logsEnabled || message == nullptr) return;

  if (firstLogAfterPrompt) {
    // Na primeira execução após habilitar o log/prompt, pula a linha para sair de baixo do "Mochi> "
    Serial.print("\n[LOG] ");
    Serial.print(message);
    firstLogAfterPrompt = false; // Desativa a quebra extra para os logs subsequentes
  } else {
    // Logs normais em sequência: colados linha a linha sem quebras extras
    Serial.print("[LOG] ");
    Serial.print(message);
  }
}

void Console::logf(const char* format, ...) {
  if (!_logsEnabled || format == nullptr) return;

  char buffer[128];
  va_list args;
  va_start(args, format);
  va_list argsCopy;
  va_copy(argsCopy, args);
  int needed = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (needed >= (int)sizeof(buffer)) {
    // Mensagem maior que o buffer local: formata em heap temporária para não truncar
    char* bigBuffer = (char*)malloc((size_t)needed + 1);
    if (bigBuffer != nullptr) {
      vsnprintf(bigBuffer, (size_t)needed + 1, format, argsCopy);
      va_end(argsCopy);
      this->log((const char*)bigBuffer);
      free(bigBuffer);
      return;
    }
  }
  va_end(argsCopy);

  if (needed < 0) return;
  this->log((const char*)buffer);
}

void Console::log(const String& message) {
  this->log(message.c_str());
}

void Console::consoleView() {
  static String inputBuffer = "";
  static bool inputOverflow = false;
  static bool lastWasCR = false; // Evita executar duas vezes em terminais CR+LF

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' && lastWasCR) {
      lastWasCR = false;
      continue;
    }
    lastWasCR = (c == '\r');

    if (c == '\n' || c == '\r') {
      if (inputOverflow) {
        menssageViewMsg("[Console] Erro: comando muito longo.");
        printPrompt();
      } else {
        inputBuffer.trim();

        if (inputBuffer.length() > 0) {
          inputBuffer.toUpperCase();
          commands_envio(inputBuffer);
        } else {
          printPrompt();
        }
      }

      inputBuffer = "";
      inputOverflow = false;
    } else if (inputBuffer.length() < MAX_INPUT_LENGTH) {
      inputBuffer += c;
    } else {
      inputOverflow = true;
    }
  }
}

void Console::commands_envio(const String& command) {
  // 1. Extrai o comando base e o parâmetro opcional caso tenha espaço (ex: "SETDELAY 0.09")
  String cmd = command;
  String paramStr = "";
  int spaceIdx = command.indexOf(' ');

  if (spaceIdx != -1) {
    cmd = command.substring(0, spaceIdx);
    paramStr = command.substring(spaceIdx + 1);
    paramStr.trim();
  }

  // Mostra o comando digitado pelo usuário
  Serial.print(_consoleText);
  Serial.println(command);

  static const CommandEntry commands[] = {
    {"HELP",        &Console::cmdHelp},
    // Controle de Logs
    {"LOGON",       &Console::cmdLogOn},
    {"LOGOFF",      &Console::cmdLogOff},
    // UI_DASHBOARD & POPUPS
    {"SHOWWIFI",    &Console::cmdWifiPopup},
    {"WIFI",        &Console::cmdWifiPopup},
    {"SHOWBRIGHT",  &Console::cmdBrightPopup},
    {"BRIGHT",      &Console::cmdBright},
    {"SETBRIGHT",   &Console::cmdSetBrightness},
    {"DISPLAYON",   &Console::cmdDisplayOn},
    {"DISPLAYOFF",  &Console::cmdDisplayOff},
    // COMANDOS DE ACIONAMENTO DE ANIMAÇÕES
    {"LOGO",        &Console::cmdLogo},
    {"ANIMACAO",    &Console::cmdAnimacao},
    {"GIF",         &Console::cmdAnimacao},
    {"TESTRED",     &Console::cmdTestRed},
    {"RED",         &Console::cmdTestRed},
    // COMANDO PARA PARAR A ANIMAÇÃO OU TESTE E RETORNAR AO DASHBOARD
    {"STOP",        &Console::cmdStop},
    {"STOPGIF",     &Console::cmdStop},
    {"DASHBOARD",   &Console::cmdStop},
    {"FORADOAR",    &Console::cmdForaDoAr},
    {"PS1",         &Console::cmdPs1},
    {"WAKE",        &Console::cmdWake},
    {"CLEAR",       &Console::cmdClear},
    {"CLS",         &Console::cmdClear},
  };

  for (const CommandEntry& entry : commands) {
    if (cmd == entry.name) {
      (this->*entry.handler)(paramStr);
      printPrompt();
      return;
    }
  }

  menssageViewMsg("[Console] Comando não reconhecido. Digite HELP para ver a lista de comandos.");
  printPrompt();
}

bool Console::animationReady() {
  if (_anim != nullptr) return true;
  menssageViewMsg("[Console] Erro: Ponteiro de Animação não registrado.");
  return false;
}

void Console::cmdHelp(const String& param) {
  (void)param;
  menssageViewMsg(
    "\n--- LISTA DE COMANDOS ---\n"
    "HELP               - Exibe a lista de comandos e funcionalidades\n"
    "LOGON              - Habilita a exibição dos logs do sistema\n"
    "LOGOFF             - Desabilita a exibição dos logs do sistema\n"
    "SHOWWIFI           - Abre/fecha alternadamente o popup de Wi-Fi\n"
    "SHOWBRIGHT         - Abre/fecha alternadamente o popup de controle do brilho\n"
    "SETBRIGHT <0-100>  - Ajusta diretamente a porcentagem de brilho no hardware PWM\n"
    "DISPLAYON          - Liga a tela (restaura o brilho anterior)\n"
    "DISPLAYOFF         - Desliga a tela (backlight em 0%)\n"
    "LOGO               - Exibe a tela de boot (LOGO Startup); repetir volta ao dashboard\n"
    "ANIMACAO <BOOT | NOTWIFI | BUG> - Executa a animação indicada\n"
    "TESTRED            - Liga/desliga a tela vermelha de teste\n"
    "FORADOAR           - Executa a animação Fora do Ar (3 s)\n"
    "STOP               - Para a animação/teste e retorna ao dashboard\n"
    "WAKE               - Liga o display manualmente no período noturno (timeout de 5 min)\n"
    "CLEAR              - Limpa as linhas e o conteúdo visível do terminal\n"
    "CLS                - Limpa as linhas e o conteúdo visível do terminal\n"
  );
}


void Console::cmdLogOn(const String& param) {
  (void)param;
  setLogState(true);
}

void Console::cmdLogOff(const String& param) {
  (void)param;
  setLogState(false);
}

void Console::cmdWifiPopup(const String& param) {
  (void)param;
  if (_popup == nullptr) {
    menssageViewMsg("[Console] Erro: Controle de popup não registrado.");
    return;
  }

  menssageViewMsg("[Console] Executando comando para Popup de Wi-Fi...");
  if (!_popup->wifiAberto()) {
    _popup->abrirWifi(); // Chamada direta para acionar a interface visual do LVGL
  } else {
    _popup->fecharWifi(); // Fechando Popup de Wi-Fi
  }
}

void Console::cmdBrightPopup(const String& param) {
  (void)param;
  if (_popup == nullptr) {
    menssageViewMsg("[Console] Erro: Controle de popup não registrado.");
    return;
  }

  if (!_popup->brilhoAberto()) {
    menssageViewMsg("[Console] Exibindo Popup de Brilho...");
    _popup->abrirBrilho();
  } else {
    menssageViewMsg("[Console] Fechando Popup de Brilho...");
    _popup->fecharBrilho();
  }
}

// "BRIGHT" sem parâmetro alterna o popup; com parâmetro ajusta o brilho (ex: BRIGHT 50)
void Console::cmdBright(const String& param) {
  if (param.length() > 0) cmdSetBrightness(param);
  else cmdBrightPopup(param);
}

// COMANDO HARDWARE PWM: Ajuste de Brilho Direto (Ex: SETBRIGHT 75 ou BRIGHT 50)
void Console::cmdSetBrightness(const String& param) {
  if (param.length() == 0) {
    menssageViewMsg("[Console] Uso: SETBRIGHT <0-100> (Ex: SETBRIGHT 80)");
    return;
  }

  if (_brightness == nullptr) {
    menssageViewMsg("[Console] Erro: Controle de brilho não registrado.");
    return;
  }

  // toInt() devolve 0 para texto inválido; por isso valida os dígitos antes
  long valor_pct = isDigitsOnly(param) ? param.toInt() : -1;
  if (valor_pct >= 0 && valor_pct <= 100) {
    _brightness->setBrilhoPorcentagem((uint8_t)valor_pct); // Aplica no hardware PWM

    char msg[64];
    snprintf(msg, sizeof(msg), "[Console] Brilho do DisplayGif ajustado para %d%%", (int)valor_pct);
    menssageViewMsg(msg);
  } else {
    menssageViewMsg("[Console] Erro: Informe um valor entre 0 e 100.");
  }
}

void Console::cmdLogo(const String& param) {
  (void)param;
  if (!animationReady()) return;

  menssageViewMsg("[Console] Executando Logo...");
  _anim->playAnimation(GifID::BOOT);
}


void Console::cmdAnimacao(const String& param) {
  (void)param;
  if (!animationReady()) return;

  bool started = true;
  if (param == "BOOT") {
    menssageViewMsg("[Console] Executando Logo...");
    started = _anim->playAnimation(GifID::BOOT);
  } else if (param == "PS1") {
    menssageViewMsg("[Console] Executando animação PS1...");
    started = _anim->playAnimation(GifID::PS1);
  } else if (param == "NOTWIFI" || param == "WIFI") {
    menssageViewMsg("[Console] Executando animação: Sem Wi-Fi...");
    started = _anim->playAnimation(GifID::NOT_WIFI);
  } else if (param == "BUG" || param == "BUG_FRAME") {
    menssageViewMsg("[Console] Executando animação: Bug Frame...");
    started = _anim->playAnimation(GifID::BUG_FRAME);
  } else {
    menssageViewMsg("[Console] Uso: ANIMACAO <BOOT | NOTWIFI | BUG>");
    return;
  }

  if (!started) menssageViewMsg("[Console] Aviso: animação indisponível (asset não habilitado ou memória insuficiente).");
}

void Console::cmdTestRed(const String& param) {
  (void)param;
  if (!animationReady()) return;

  if (_anim->isPlaying()) {
    _anim->stop(); // Para o teste/animação em andamento e volta ao Dashboard
  } else {
    menssageViewMsg("[Console] Executando teste: Tela Vermelha...");
    _anim->testRedScreen(); // Executa o teste na classe Animations
  }
}

void Console::cmdStop(const String& param) {
  (void)param;
  if (!animationReady()) return;

  _anim->stop(); // Chama o stop() que força o redesenho no LVGL
  _popup->fecharWifi();
  _popup->fecharBrilho();

  menssageViewMsg("[Console] Teste/Animação parada. Retornando ao Dashboard LVGL.");
}

void Console::cmdForaDoAr(const String& param) {
  (void)param;
  if (!animationReady()) return;

  if (_anim->isPlaying() && _anim->getCurrentAnimation() == GifID::FORA_DO_AR) {
    _anim->stop();
  } else {
    menssageViewMsg("[Console] Executando animação Fora do Ar...");
    _anim->foraDoAr(); // Executa o teste na classe Animations
  }
}

void Console::cmdPs1(const String& param) {
  (void)param;
  if (!animationReady()) return;

  if (_anim->isPlaying() && _anim->getCurrentAnimation() == GifID::PS1) {
    _anim->stop();
  } else {
    menssageViewMsg("[Console] Executando animação PS1...");
    //_anim->ps1(); // Executa o teste na classe Animations
    //_anim->playAnimation(GifID::PS1);
  }
}



// DISPLAYON/DISPLAYOFF passam pelo Hours_Time (quando existe) para respeitar o período noturno
void Console::cmdDisplayOn(const String& param) {
  (void)param;
  Hours_Time* horario = Hours_Time::instance();
  if (horario != nullptr) {
    horario->manual_turn_on(); // No período noturno volta a dormir em 5 minutos
  } else if (_brightness != nullptr) {
    _brightness->displayOn(); // Restaura o brilho memorizado no FileSystemControll
  } else {
    menssageViewMsg("[Console] Erro: Controle de brilho não registrado.");
    return;
  }

  menssageViewMsg("[Console] Display ligado.");
}

void Console::cmdDisplayOff(const String& param) {
  (void)param;
  Hours_Time* horario = Hours_Time::instance();
  if (horario != nullptr) {
    horario->manual_turn_off();
  } else if (_brightness != nullptr) {
    _brightness->displayOff(); // Apaga o backlight zerando o PWM via FileSystemControll
  } else {
    menssageViewMsg("[Console] Erro: Controle de brilho não registrado.");
    return;
  }

  menssageViewMsg("[Console] Display desligado.");
}

void Console::cmdWake(const String& param) {
    // Handle the WAKE command here
}

void Console::cmdClear(const String& param) {
    (void)param;
    Serial.print("\033[2J\033[H");
    menssageViewMsg("[Console] Terminal limpo com sucesso.");
}

