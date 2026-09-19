#!/usr/bin/env bash

# ==============================================================================
# Script: configurar.sh
# Descrição: Atualiza em tempo real o Endpoint e o Timeout do telemetry.sh
# ==============================================================================

set -euo pipefail

# Arquivo alvo a ser reconfigurado
readonly TARGET_SCRIPT="telemetry.sh"
readonly TARGET_SERVICE="/etc/systemd/system/telemetry.service"

# ------------------------------------------------------------------------------
# Validação do Arquivo de Telemetry
# ------------------------------------------------------------------------------
if [[ ! -f "$TARGET_SCRIPT" ]]; then
    printf "[ERRO] Arquivo '%s' não encontrado no diretório atual!\n" "$TARGET_SCRIPT" >&2
    exit 1
fi

# ------------------------------------------------------------------------------
# Leitura dos Parâmetros Atuais do telemetry.sh
# ------------------------------------------------------------------------------
CURRENT_SERVER_NAME=$(grep -E '^readonly DEFAULT_SERVER_NAME=' "$TARGET_SCRIPT" | cut -d'"' -f2 || echo "")
CURRENT_ENDPOINT=$(grep -E '^readonly DEFAULT_ENDPOINT=' "$TARGET_SCRIPT" | cut -d'"' -f2 || echo "http://192.168.1.50/api/telemetry")
CURRENT_LOGO=$(grep -E '^readonly DEFAULT_LOGO=' "$TARGET_SCRIPT" | cut -d'"' -f2 || echo "https://assets.ubuntu.com/v1/29383635-ubuntu-logo-2022.png")
CURRENT_TIMEOUT=$(grep -E '^readonly DEFAULT_TIMEOUT=' "$TARGET_SCRIPT" | cut -d'=' -f2 || echo "5")

# ------------------------------------------------------------------------------
# Interface Interativa de Terminal
# ------------------------------------------------------------------------------
printf "\n==================================================\n"
printf "       CONFIGURAÇÃO EM TEMPO REAL - TELEMETRy\n"
printf "==================================================\n\n"

# 1. Prompt para URL do Servidor
read -r -p "URL Server [$CURRENT_ENDPOINT]: " INPUT_ENDPOINT
NEW_ENDPOINT="${INPUT_ENDPOINT:-$CURRENT_ENDPOINT}"

# 2. Prompt para Tempo de Timeout
read -r -p "Tempo de Timeout (segundos) [$CURRENT_TIMEOUT]: " INPUT_TIMEOUT
NEW_TIMEOUT="${INPUT_TIMEOUT:-$CURRENT_TIMEOUT}"

# 3. Prompt para Nome do Host/Server
read -r -p "Nome do Host/Server (vazio para usar hostname do SO) [$CURRENT_SERVER_NAME]: " INPUT_SERVER_NAME
NEW_SERVER_NAME="${INPUT_SERVER_NAME:-$CURRENT_SERVER_NAME}"

# 4. Prompt para URL do Logo/Imagem
read -r -p "URL do Logo/Imagem [$CURRENT_LOGO]: " INPUT_LOGO
NEW_LOGO="${INPUT_LOGO:-$CURRENT_LOGO}"

# ------------------------------------------------------------------------------
# Alteração em Tempo Real no telemetry.sh via sed
# ------------------------------------------------------------------------------
# O caractere '|' é usado como delimitador para evitar conflito com os barra '/' da URL
sed -i "s|^readonly DEFAULT_ENDPOINT=.*|readonly DEFAULT_ENDPOINT=\"${NEW_ENDPOINT}\"|" "$TARGET_SCRIPT"
sed -i "s|^readonly DEFAULT_TIMEOUT=.*|readonly DEFAULT_TIMEOUT=${NEW_TIMEOUT}|" "$TARGET_SCRIPT"
sed -i "s|^readonly DEFAULT_SERVER_NAME=.*|readonly DEFAULT_SERVER_NAME=\"${NEW_SERVER_NAME}\"|" "$TARGET_SCRIPT"
sed -i "s|^readonly DEFAULT_LOGO=.*|readonly DEFAULT_LOGO=\"${NEW_LOGO}\"|" "$TARGET_SCRIPT"

# ------------------------------------------------------------------------------
# Atualização opcional no /etc/systemd/system/telemetry.service e Restart
# ------------------------------------------------------------------------------
if [[ -f "$TARGET_SERVICE" ]]; then
    sudo sed -i "s|Environment=ENDPOINT_URL=.*|Environment=ENDPOINT_URL=\"${NEW_ENDPOINT}\"|" "$TARGET_SERVICE"
    sudo systemctl daemon-reload
    sudo systemctl restart telemetry.service
    printf "[INFO] Serviço telemetry.service atualizado e reiniciado com sucesso.\n"
else
    printf "[AVISO] Arquivo de serviço '%s' não encontrado. Pulando atualização do systemd.\n" "$TARGET_SERVICE"
fi

# ------------------------------------------------------------------------------
# Confirmação Final
# ------------------------------------------------------------------------------
printf "\n==================================================\n"
printf "✔ Os dados foram alterados com sucesso!\n"
printf "==================================================\n"
printf "  • Nome Host  : %s\n" "${NEW_SERVER_NAME:-Automático ($(hostname 2>/dev/null || echo "Desconhecido"))}"
printf "  • URL Server : %s\n" "$NEW_ENDPOINT"
printf "  • URL Logo   : %s\n" "$NEW_LOGO"
printf "  • Timeout    : %ss\n" "$NEW_TIMEOUT"
printf "==================================================\n\n"