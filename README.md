#### Ideias
1. se após 5 minutos não hover nunum payload vai entrar uma proteção de tela.


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

### 1. Crie o arquivo de serviço do systemd:

```bash
sudo nano /etc/systemd/system/telemetry.service
```

### 2. Cole o conteúdo abaixo (ajuste o usuário se necessário):

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

### 3. Recarregue os serviços e inicie o monitoramento:

```bash
# Recarrega o gerenciador do systemd
sudo systemctl daemon-reload

# Inicia e habilita na inicialização do SO
sudo systemctl enable --now telemetry.service

# Verifica o status e os logs de execução
sudo systemctl status telemetry.service

# Verificar log em tempo real

```

### 4. Copie co codigo da versão do sistema operacional na raiz do porjeto.

## Arquivo de Configuração no Linux

### 2. configurar.sh
#### Script utilitário interativo para reconfiguração dos parâmetros de rede e tempo real sem edição manual de arquivos.

### 1. Criar e Salvar o Arquivo
Certifique-se de estar na mesma pasta onde está o arquivo telemetry.sh:

```bash
nano configurar.sh
```

Cole o código acima, pressione Ctrl + O, aperte Enter para salvar e Ctrl + X para sair.

### 2. Dar Permissão de Execução

```bash
chmod +x configurar.sh
```

### 3. Executar o Script de Configuração

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


Persona: DESENVOLVEDOR FULL STAKER EM C/C++

CONTEXTO:
criação de classe para exibição de gif animados.

AÇÃO:
cria a classe Animations, que vai realizar a criação de animações no display 2.8 inch tft, crie uma classe inteligente evitando
loop que possam travar o modulo, mas exibindo de maneira leve.

essa classe vai alternar junto ao deshboard, assim que ela for chamada o dasbord sai da tela, assim que a execução for parada volta ao deshboard.

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

Persona: DESENVOLVEDOR FULL STAKER EM C/C++

CONTEXTO:
esta é a base de toda a telementria.

AÇÃO:
verifiquei que a metrica de get_disk_metrics se da apenas sobre a metrica dos partições, e esse não é o objetivo, 
quero está verificando todo o conteudo dos discos em um sistema opracional. tenha em mente que as metricas de esp32(sistemas embarcado não pode ser modificado), não pode se perder, o dashboard lvjl tem que está completamente alinhado ao dashboard lvgl, recebendo o mesmo payload, é estramamnete importante não modificar nome de variaveis, funcções e comantarios. e o layout do html, permaneça intacto, apenas incluindo o dasdos do hd.

vou postar em anexo a base dos arquivos. e um trecho de codigo para avaliar como uma ideia.

``bash
get_hardware_disk_metrics() {
    local has_smartctl=0
    if command -v smartctl >/dev/null 2>&1; then
        has_smartctl=1
    fi

    local df_output
    df_output=$(df -B1 2>/dev/null || true)

    echo "["
    local first=1

    for disk_path in /sys/block/*; do
        [ -e "$disk_path" ] || continue
        local disk_name=$(basename "$disk_path")

        if [[ "$disk_name" =~ ^(loop|ram|zram|sr|fd|nbd|dm-) ]]; then
            continue
        fi

        if [[ "$disk_name" =~ ^(sd|hd|vd)[a-z]+$ ]]; then
            :
        elif [[ "$disk_name" =~ ^nvme[0-9]+n[0-9]+$ ]]; then
            :
        elif [[ "$disk_name" =~ ^mmcblk[0-9]+$ ]]; then
            :
        else
            continue
        fi

        local size_sectors=$(cat "$disk_path/size" 2>/dev/null || echo 0)
        local total_bytes=$((size_sectors * 512))

        if [ "$total_bytes" -eq 0 ]; then
            continue
        fi

        local model="Desconhecido"
        if [ -f "$disk_path/device/model" ]; then
            model=$(cat "$disk_path/device/model" 2>/dev/null)
        elif [ -f "$disk_path/device/name" ]; then
            model=$(cat "$disk_path/device/name" 2>/dev/null)
        fi
        model=$(echo "$model" | tr -d '"\\\r\n' | xargs)
        [ -z "$model" ] && model="Desconhecido"

        local usado_bytes=0
        usado_bytes=$(echo "$df_output" | awk -v disk="$disk_name" '
            $1 ~ "^/dev/" disk { used += $3 }
            END { print used+0 }
        ')

        local livre_bytes=$((total_bytes - usado_bytes))
        if [ "$livre_bytes" -lt 0 ]; then
            livre_bytes=0
        fi

        local health="UNKNOWN"
        if [ "$has_smartctl" -eq 1 ]; then
            local smart_out
            smart_out=$(sudo smartctl -H "/dev/$disk_name" 2>&1 || true)

            if echo "$smart_out" | grep -iqE "PASSED|OK|HEALTHY"; then
                health="PASSED"
            elif echo "$smart_out" | grep -iq "FAILED"; then
                health="FAILED"
            fi
        fi

        if [ "$first" -eq 1 ]; then
            first=0
        else
            echo ","
        fi

        cat <<EOF
  {
    "device": "/dev/$disk_name",
    "model": "$model",
    "total_bytes": $total_bytes,
    "usado_bytes": $usado_bytes,
    "livre_bytes": $livre_bytes,
    "health_status": "$health"
  }
EOF
    done
    echo "]"
}
```

AÇÃO:
Realize uma análise técnica detalhada do código avaliando:

- Legibilidade
- Manutenibilidade
- Performance
- Possíveis bugs
- Tratamento de exceções
- Acoplamento e coesão
- Princípios SOLID

