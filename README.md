#### Ideias
1. Colocar as % dentro do circuilo no dashbord.
2. se após 5 minutos não hover nunum payload vai entrar uma proteção de tela.


# Telemetria & Monitoramento de Servidores (ESP32 + Linux Bash)

Sistema completo de monitoramento e telemetria para servidores Linux com exibição em tempo real em um display TFT ILI9341 de 2.8" gerenciado por um ESP32. O sistema é composto por um coletor de dados em Bash (executado como serviço no Linux) e um receptor Web embarcado no ESP32 com suporte a Auto-Discovery e ajuste dinâmico de configurações.

# 🚀 Funcionalidades do Projeto

* Coleta Nativa de Telemetria no Linux (telemetry.sh):

* * Métricas do Sistema: Identificação do Host, IP local, MAC Address, Uptime formatado, versão do Kernel e Arquitetura do SO.

* * Hardware & Desempenho: Temperatura da CPU (°C), Carga da CPU (Load Average de 1 min), consumo de Memória RAM (Bytes brutos) e ocupação de Disco (Bytes brutos).

* * Identificação Visual: Detecção automática do SO (Ubuntu, Raspbian, Linux genérico) com URL de logotipo dinâmico.

* * Payload JSON Padronizado: Estrutura contendo dados brutos para que a interface (front-end) converta e formate as unidades de medida conforme necessário.

* Auto-Discovery e Recuperação Automática de Conexão:

* * Em caso de falha de comunicação com o endpoint configurado, o script realiza uma varredura automática na sub-rede (/24) disparando requisições em /api/autodiscovery.

* * Assim que o ESP32 responde com {"status":"ok"}, o endereço IP é atualizado automaticamente no arquivo local config.env e o envio é restaurado sem intervenção manual.

* Configuração em Tempo Real (configurar.sh):

* * Interface interativa via terminal para alterar URL do servidor, timeout de requisição, nome do host e URL da logo.

* * Atualização dinâmica via sed no script principal e re-sincronização automática do serviço no Systemd (telemetry.service).

* Automação via Systemd (telemetry.service):

* * Execução contínua em segundo plano no Linux, com inicialização automática no boot do sistema operacional e reinicialização automática após falhas.

* Dashboard em Tela TFT SPI (ESP32 + ILI9341 2.8"):

* * Conexão via barramento VSPI/SPI nativo com controle de brilho via PWM.

* * Texto Padrão de Inicialização (Fallback): Exibição de mensagem/status padrão na tela ao ligar o ESP32 antes do recebimento do primeiro payload de dados.


### 🛠️ Configurações de Tela & Hardware (ESP32)
### Mapeamento de Pinos (Display ILI9341 2.8" SPI)

| Pino do Display ILI9341 | Pino no ESP32 | Definição no Código | Função / Observação |
| :--- | :--- | :--- | :--- |
| VCC | 3.3V / 5V | Alimentação | Alimentação do módulo |
| GND | GND | Terra | Terra comum |
| CS | GPIO 15(ou GPIO 5) | TFT_CS | Chip Select SPI |
| RESET	| GPIO 4 | TFT_RST | Reset do Display |
| DC / RS | GPIO 2 | TFT_DC | Data / Command Select |
| SDI / MOSI | GPIO 23 |TFT_MOSI | Dados SPI (Master Out) |
| SCK / CLK | GPIO 18 | TFT_SCLK | Relógio SPI (Clock) |
| LED / BL | GPIO 21 | TFT_BL | Controle de Backlight (PWM / Transistor) |
| SDO / MISO | GPIO 19 | TFT_MISO | Dados SPI (Master In) |

```c++
// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15 | 5  // Chip select control pin (ou GPIO 5)
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
//#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
#define TFT_BL   21  // Backlight control pin
```

### 🐧 Instalação e Configuração no Linux

#### Passo 1: Criar a Pasta do Projeto
Crie uma pasta dedicada para o script de monitoramento e acesse-a:
```bash
# Cria o diretório ~/telemetry
mkdir -p ~/telemetry

# Entra na pasta criada
cd ~/telemetry
```

#### Passo 2: Criar e Salvar o Arquivo .sh
Você pode utilizar o editor de texto nano ou criar o arquivo diretamente via linha de comando (cat):

#### Opção A: Usando o editor nano

1. Abra o editor criando o arquivo:

```bash
nano telemetry.sh
```

2. Cole o código completo da versão profissional do script.

3. Para salvar no nano: pressione Ctrl + O, confirme com Enter e depois saia com Ctrl + X.

#### Passo 3: Dar Permissão de Execução ao Script
Para que o sistema operacional permita executar o arquivo como um programa, conceda a permissão +x:

```bash
chmod +x telemetry.sh
```

#### Passo 5: Testar e Executar o Script
1. Execução Simples (Teste)

```bash
./telemetry.sh
```

#### Serviço de Sistema via Systemd (Recomendado para Produção)
Para garatir que o script inicie automaticamente com o boot do SO e reinicie em caso de falha:

1. Crie o arquivo de serviço do systemd:

```bash
sudo nano /etc/systemd/system/telemetry.service
```

2. Cole o conteúdo abaixo (ajuste o usuário se necessário):

```bash
[Unit]
Description=Servico de Coleta de telemetry
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/telemetry
ExecStart=/bin/bash /home/pi/telemetry/telemetry.sh
Restart=always
RestartSec=10
Environment=ENDPOINT_URL="http://192.168.1.6/api/telemetry"

[Install]
WantedBy=multi-user.target
```

3. Recarregue os serviços e inicie o monitoramento:

```bash
# Recarrega o gerenciador do systemd
sudo systemctl daemon-reload

# Inicia e habilita na inicialização do SO
sudo systemctl enable --now telemetry.service

# Verifica o status e os logs de execução
sudo systemctl status telemetry.service

# Verificar log em tempo real

```

### 📜 Código dos Scripts
#### 1. telemetry.sh
```bash
#!/usr/bin/env bash

# ==============================================================================
# Script: telemetry.sh
# Descrição: Coletor de telemetry nativo em Bash adaptado ao novo schema JSON com Auto-Discovery.
# ==============================================================================

set -euo pipefail
LC_ALL=C # Força separador decimal padrão (ponto)

# ------------------------------------------------------------------------------
# Configurações Padrão & Variáveis de Ambiente
# ------------------------------------------------------------------------------
readonly DEFAULT_ENDPOINT="http://192.168.1.6/api/telemetry"
readonly DEFAULT_TIMEOUT=5
readonly DEFAULT_LOGO="https://cdn-icons-png.flaticon.com/512/518/518713.png" # Logo Padrão/Genérica
readonly DEFAULT_LOGO_UBUNTU="https://assets.ubuntu.com/v1/29383635-ubuntu-logo-2022.png"
readonly DEFAULT_LOGO_RASPBERRY="https://www.raspberrypi.com/app/uploads/2020/06/raspberrry_pi_logo.png"
readonly DEFAULT_SERVER_NAME="Server-RBP"

SERVER_NAME="${SERVER_NAME:-$DEFAULT_SERVER_NAME}"
ENDPOINT_URL="${ENDPOINT_URL:-$DEFAULT_ENDPOINT}"
TIMEOUT_SEC="${TIMEOUT_SEC:-$DEFAULT_TIMEOUT}"
OS_LOGO_URL="${OS_LOGO_URL:-}"


# ------------------------------------------------------------------------------
# Configuração Externa (Garante que o config.env exista)
# ------------------------------------------------------------------------------
CONFIG_FILE="/home/pi/telemetry/config.env"
DEFAULT_FALLBACK_ENDPOINT="http://192.168.1.6/api/telemetry"

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "ENDPOINT_URL=\"$DEFAULT_FALLBACK_ENDPOINT\"" > "$CONFIG_FILE"
    log_info "Arquivo config.env criado automaticamente com o endpoint padrão."
fi

# Carrega a variável ENDPOINT_URL do arquivo de configuração
source "$CONFIG_FILE"
ENDPOINT_URL="${ENDPOINT_URL:-$DEFAULT_FALLBACK_ENDPOINT}"

# ------------------------------------------------------------------------------
# Logging e Validações
# ------------------------------------------------------------------------------
log_info()  { printf "[INFO]  %s - %s\n" "$(date -u +'%Y-%m-%dT%H:%M:%SZ')" "$*" >&2; }
log_error() { printf "[ERROR] %s - %s\n" "$(date -u +'%Y-%m-%dT%H:%M:%SZ')" "$*" >&2; }

check_dependencies() {
    if ! command -v curl &>/dev/null; then
        log_error "Dependência ausente no sistema: curl"
        exit 1
    fi
}

# ------------------------------------------------------------------------------
# Funções Coletoras de Dados
# ------------------------------------------------------------------------------
get_mac() {
    local iface mac
    iface=$(ip route show default 2>/dev/null | awk '/default/ {print $5}' | head -n 1)
    mac=$(cat /sys/class/net/"${iface:-eth0}"/address 2>/dev/null | tr '[:lower:]' '[:upper:]' || echo "00:00:00:00:00:00")
    echo "${mac:-00:00:00:00:00:00}"
}

get_ip() {
    local ip
    ip=$(ip route get 1.1.1.1 2>/dev/null | grep -oP 'src \K\S+' || hostname -I | awk '{print $1}')
    echo "${ip:-0.0.0.0}"
}

get_id() {
    local mac="$1"
    echo "$mac" | tr -d ':-'
}

get_os_info() {
    local name="Linux" logo="$DEFAULT_LOGO"
    if [[ -f /etc/os-release ]]; then
        name=$(source /etc/os-release 2>/dev/null && echo "$PRETTY_NAME") || name=$(uname -s)
    else
        name=$(uname -s)
    fi

    if [[ "$name" =~ [Uu]buntu ]]; then
        logo="$DEFAULT_LOGO_UBUNTU"
    elif [[ "$name" =~ [Rr]aspbian|[Rr]aspberry ]]; then
        logo="$DEFAULT_LOGO_RASPBERRY"
    fi

    echo "$name|$logo"
}

get_cpu_temp() {
    local temp="0.0"
    if [[ -f /sys/class/thermal/thermal_zone0/temp ]]; then
        local raw_temp
        raw_temp=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null || echo "0")
        temp=$(awk -v t="$raw_temp" 'BEGIN {printf "%.1f", t/1000}')
    elif command -v vcgencmd &>/dev/null; then
        temp=$(vcgencmd measure_temp 2>/dev/null | grep -oE '[0-9.]+' || echo "0.0")
    fi
    echo "${temp:-0.0}"
}

get_cpu_load() {
    if [[ -f /proc/loadavg ]]; then
        read -r load1 _ < /proc/loadavg
        echo "$load1"
    else
        uptime | awk -F'load average:' '{ print $2 }' | awk '{ print $1 }' | tr -d ','
    fi
}

get_uptime_formatted() {
    if [[ -f /proc/uptime ]]; then
        local uptime_sec days hours mins
        uptime_sec=$(cut -d' ' -f1 /proc/uptime | cut -d'.' -f1)
        days=$(( uptime_sec / 86400 ))
        hours=$(( (uptime_sec % 86400) / 3600 ))
        mins=$(( (uptime_sec % 3600) / 60 ))

        printf "%dd %02dh %02dm" "$days" "$hours" "$mins"
    else
        uptime -p | sed 's/up //'
    fi
}

get_memory_metrics() {
    # Coleta valores brutos em Bytes diretamente do /proc/meminfo para o Front-end converter
    local total_bytes=0 available_bytes=0 used_bytes=0
    if [[ -f /proc/meminfo ]]; then
        local mem_total_kb mem_avail_kb
        mem_total_kb=$(awk '/MemTotal:/ {print $2}' /proc/meminfo || echo "0")
        mem_avail_kb=$(awk '/MemAvailable:/ {print $2}' /proc/meminfo || echo "")

        if [[ -z "$mem_avail_kb" ]]; then
            local mem_free_kb mem_buffers_kb mem_cached_kb
            mem_free_kb=$(awk '/MemFree:/ {print $2}' /proc/meminfo || echo "0")
            mem_buffers_kb=$(awk '/Buffers:/ {print $2}' /proc/meminfo || echo "0")
            mem_cached_kb=$(awk '/^Cached:/ {print $2}' /proc/meminfo || echo "0")
            mem_avail_kb=$(( ${mem_free_kb:-0} + ${mem_buffers_kb:-0} + ${mem_cached_kb:-0} ))
        fi

        total_bytes=$(( mem_total_kb * 1024 ))
        available_bytes=$(( mem_avail_kb * 1024 ))
        used_bytes=$(( total_bytes - available_bytes ))
    fi
    echo "$total_bytes $used_bytes $available_bytes"
}

get_disk_metrics() {
    # Coleta valores brutos em Bytes do sistema de arquivos para o Front-end manipular
    local total_bytes=0 used_bytes=0 free_bytes=0
    if df_out=$(df -B1 / 2>/dev/null | awk 'END {print $2, $3, $4}'); then
        read -r total_bytes used_bytes free_bytes <<< "$df_out"
    fi
    echo "${total_bytes:-0} ${used_bytes:-0} ${free_bytes:-0}"
}

# ------------------------------------------------------------------------------
# Função de Auto-Discovery (Varredura de Rede via /api/autodiscovery)
# ------------------------------------------------------------------------------
auto_discovery_esp32() {
    local default_route base_ip i target response telemetry_target
    local config_file="/home/pi/telemetry/config.env"
    
    default_route=$(ip route show default 2>/dev/null | awk '/default/ {print $3}' || echo "")
    if [[ -z "$default_route" ]]; then
        default_route=$(hostname -I | awk '{print $1}')
    fi
    base_ip=$(echo "$default_route" | cut -d'.' -f1-3)
    
    if [[ -z "$base_ip" ]]; then
        return 1
    fi
    
    log_info "Varredura iniciada na rede ${base_ip}.0/24 via /api/autodiscovery..."
    for i in {1..254}; do
        target="http://$base_ip.$i/api/autodiscovery"
        response=$(curl -s --max-time 0.2 -X POST "$target" \
            -H "Content-Type: application/json" \
            -d '{"host":"AUTO_DISCOVERY_TEST"}' 2>/dev/null || echo "")
        
        if [[ "$response" == *'"status":"ok"'* ]] || [[ "$response" == *'status: ok'* ]]; then
            telemetry_target="http://$base_ip.$i/api/telemetry"
            ENDPOINT_URL="$telemetry_target"
            log_info "ESP32 encontrado! Novo endpoint: $telemetry_target"
            
            # Atualiza de forma 100% segura apenas o arquivo de configuração separado
            echo "ENDPOINT_URL=\"$telemetry_target\"" > "$config_file"
            log_info "Arquivo config.env atualizado com o novo IP."
            
            return 0
        fi
    done
    log_error "Nenhum dispositivo respondeu ao autodiscovery na rede."
    return 1
}

# ------------------------------------------------------------------------------
# Montagem do Payload no novo formato JSON com Dados Brutos
# ------------------------------------------------------------------------------
build_telemetry_payload() {
    local id ip mac host uptime timestamp os_name os_logo kernel arch cpu_temp cpu_load mem_total_bytes mem_used_bytes mem_free_bytes disk_total_bytes disk_used_bytes disk_free_bytes os_info datetime epoch_timestamp

    mac=$(get_mac)
    ip=$(get_ip)
    id=$(get_id "$mac")

    host="${SERVER_NAME:-$(hostname 2>/dev/null || echo "Desconhecido")}"
    uptime=$(get_uptime_formatted)
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    datetime=$(date +"%Y-%m-%d %H:%M:%S")
    epoch_timestamp=$(date +%s)

    os_info=$(get_os_info)
    os_name="${os_info%%|*}"
    os_logo="${OS_LOGO_URL:-${os_info##*|}}"

    kernel=$(uname -r)
    arch=$(uname -m)
    cpu_temp=$(get_cpu_temp)
    cpu_load=$(get_cpu_load)

    read -r mem_total_bytes mem_used_bytes mem_free_bytes <<< "$(get_memory_metrics)"
    read -r disk_total_bytes disk_used_bytes disk_free_bytes <<< "$(get_disk_metrics)"

    cpu_temp="${cpu_temp:-0.0}"
    cpu_load="${cpu_load:-0.0}"
    mem_total_bytes="${mem_total_bytes:-0}"
    mem_used_bytes="${mem_used_bytes:-0}"
    mem_free_bytes="${mem_free_bytes:-0}"
    disk_total_bytes="${disk_total_bytes:-0}"
    disk_used_bytes="${disk_used_bytes:-0}"
    disk_free_bytes="${disk_free_bytes:-0}"

    cat <<EOF
{
  "id": "$id",
  "ip": "$ip",
  "mac": "$mac",
  "host": "$host",
  "timestamp": "$timestamp",
  "datetime": "$datetime",
  "epoch_timestamp": $epoch_timestamp,
  "uptime": "$uptime",
  "sistema_operacional": {
    "nome": "$os_name",
    "kernel": "$kernel",
    "arquitetura": "$arch",
    "logo_url": "$os_logo"
  },
  "metricas": {
    "cpu_temp": $cpu_temp,
    "cpu_load_1m": $cpu_load,
    "memoria": {
      "total_bytes": $mem_total_bytes,
      "usada_bytes": $mem_used_bytes,
      "livre_bytes": $mem_free_bytes
    },
    "disco": {
      "total_bytes": $disk_total_bytes,
      "usado_bytes": $disk_used_bytes,
      "livre_bytes": $disk_free_bytes
    }
  }
}
EOF
}

# ------------------------------------------------------------------------------
# Envio do Payload
# ------------------------------------------------------------------------------
send_payload() {
    local payload="$1"
    local http_code

    log_info "Enviando telemetry para $ENDPOINT_URL..."

    http_code=$(curl -s -o /dev/null -w "%{http_code}" \
        --connect-timeout "$TIMEOUT_SEC" \
        --max-time $((TIMEOUT_SEC * 2)) \
        -X POST "$ENDPOINT_URL" \
        -H "Content-Type: application/json" \
        -d "$payload" || echo "000")

    if [[ "$http_code" -ge 200 && "$http_code" -lt 300 ]]; then
        log_info "Enviado com sucesso! Status HTTP: $http_code"
        return 0
    else
        log_error "Falha no envio. Status HTTP: $http_code"
        return 1
    fi
}

main() {
    check_dependencies

    local payload
    payload=$(build_telemetry_payload)

    if ! send_payload "$payload"; then
        log_info "Tentando recuperar a conexão através do Auto-Discovery..."
        if auto_discovery_esp32; then
            if send_payload "$payload"; then
                exit 0
            fi
        fi
        exit 1
    fi
}

main "$@"
```

### 2. configurar.sh
#### Script utilitário interativo para reconfiguração dos parâmetros de rede e tempo real sem edição manual de arquivos.

1. Criar e Salvar o Arquivo
Certifique-se de estar na mesma pasta onde está o arquivo telemetry.sh:

```bash
nano configurar.sh
```

Cole o código acima, pressione Ctrl + O, aperte Enter para salvar e Ctrl + X para sair.

### 2. Dar Permissão de Execução

```bash
chmod +x configurar.sh
```

#### 3. Executar o Script de Configuração

```bash
./configurar.sh
```

Aqui está o script de configuração interativa em tempo real (configurar.sh).

Ele lê automaticamente as configurações vigentes do seu script telemetry.sh, exibe-as entre colchetes como padrão se o usuário apenas pressionar Enter, e atualiza o arquivo de telemetry com os novos valores usando o sed.

#### Script de Configuração (configurar.sh)

```bash
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
```


```
Persona: DESENVOLVEDOR FULL STAKER EM C/C++

CONTEXTO:
Ajude na avaliação deste projeto.

AÇÃO:
Realize uma análise técnica detalhada do código avaliando:

- Legibilidade
- Manutenibilidade
- Performance
- Possíveis bugs
- Tratamento de exceções
- Acoplamento e coesão
- Princípios SOLID
- Boas práticas de Selenium

RESTRIÇÕES:

- Não alterar nomes de variáveis.
- Não alterar nomes de funções.
- Não alterar comentários existentes.
- Não reescrever o código completo.
- Não gerar versão refatorada sem solicitação.
- Apresentar primeiro a análise técnica.
- Apresentar depois as sugestões separadamente.
- Utilizar respostas objetivas e pouco verbosas.

FALHAS:
até o momento não renderza o dashbord que foi proposto na imagem gerada.
se não hover um payload na icialização do esp32, tenha um texto padrão


Persona: DESENVOLVEDOR FULL STAKER EM SHELL/BASH

CONTEXTO:
remonte a ultima versão do arquivo shell para linux, com todas as variave, funções e comentários.

AÇÃO:
Realize uma análise técnica detalhada do código avaliando:

- Legibilidade
- Manutenibilidade
- Performance
- Possíveis bugs
- Tratamento de exceções
- Acoplamento e coesão
- Princípios SOLID
```


Persona: DESENVOLVEDOR FULL STAKER EM C/C++

CONTEXTO:
Quero realizar troca no layout do dashbord LVGL, sem modificar suas caracteristicas.

AÇÃO:
- quero em cada card, por o circuilo no maio com a porcentagem no meio do circuilo e retire o texto "de 100%".

vou enviar junto o arquivo .h para Verificar.

AÇÃO:
Realize uma análise técnica detalhada do código avaliando:

- Legibilidade
- Manutenibilidade
- Performance
- Possíveis bugs
- Tratamento de exceções
- Acoplamento e coesão
- Princípios SOLID


Persona: DESENVOLVEDOR FULL STAKER EM HTML/CSS

CONTEXTO:
com os mesmo padrões que foi criado a pagina principal do porjeto de telemetria, ajuste a pagina de configurações que conterão estas tegs.

AÇÃO:
Realize uma análise técnica detalhada do código avaliando:

- Legibilidade
- Manutenibilidade
- Performance
- Possíveis bugs
- Tratamento de exceções
- Acoplamento e coesão
- Princípios SOLID
```
