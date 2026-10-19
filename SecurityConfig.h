#ifndef SECURITYCONFIG_H
#define SECURITYCONFIG_H

// Segredos e opções de segurança ficam em "secrets.h" (ignorado pelo Git).
// Copie "secrets.h.example" para "secrets.h" e preencha. Sem o arquivo, valem os padrões abaixo.
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

// Senha do portal de configuração (AP). Mínimo de 8 caracteres (WPA2).
#ifndef AP_PASSWORD_DEFAULT
  #define AP_PASSWORD_DEFAULT ""
#endif

// Token exigido no header "X-Auth-Token" em POST /api/telemetry e POST /api/config.
// Vazio = autenticação desativada (compatível com os coletores atuais).
#ifndef API_TOKEN
  #define API_TOKEN ""
#endif

// Origem permitida para CORS. Vazio = sem headers CORS (somente mesma origem).
#ifndef CORS_ALLOW_ORIGIN
  #define CORS_ALLOW_ORIGIN ""
#endif

#endif // SECURITYCONFIG_H