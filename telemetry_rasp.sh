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
readonly DEFAULT_LOGO_WINDOWS="https://upload.wikimedia.org/wikipedia/commons/6/6d/Windows_Logo_%281992-2001%29.svg"
readonly DEFAULT_LOGO_LINUX="https://upload.wikimedia.org/wikipedia/commons/d/d6/Linux_mascot_tux.png"
readonly DEFAULT_SERVER_NAME="Server-RBP"

SERVER_NAME="${SERVER_NAME:-$DEFAULT_SERVER_NAME}"
ENDPOINT_URL="${ENDPOINT_URL:-$DEFAULT_ENDPOINT}"
TIMEOUT_SEC="${TIMEOUT_SEC:-$DEFAULT_TIMEOUT}"
OS_LOGO_URL="${OS_LOGO_URL:-}"


# ------------------------------------------------------------------------------
# Configuração Externa (Garante que o config.env exista)
# ------------------------------------------------------------------------------
CONFIG_DIR="${HOME}/telemetry"
CONFIG_FILE="${CONFIG_DIR}/config.env"
DEFAULT_FALLBACK_ENDPOINT="http://192.168.1.6/api/telemetry"

if [[ ! -f "$CONFIG_FILE" ]]; then
    mkdir -p "$CONFIG_DIR" 2>/dev/null || true
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
    ip=$(ip route get 1.1.1.1 2>/dev/null | awk '{print $7}' || hostname -I | awk '{print $1}')
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
    # Garante estritamente que qualquer vírgula seja convertida para ponto
    echo "${temp:-0.0}" | tr ',' '.'
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
            $1 ~ "^/dev/" disk "([0-9]|p[0-9]|$)" { used += $3 }
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

get_disk_metrics() {
    # Coleta valores brutos em Bytes agregando todos os discos físicos do sistema
    local total_bytes=0 used_bytes=0 free_bytes=0
    local df_output
    df_output=$(df -B1 2>/dev/null || true)

    for disk_path in /sys/block/*; do
        [ -e "$disk_path" ] || continue
        local disk_name=$(basename "$disk_path")

        if [[ "$disk_name" =~ ^(loop|ram|zram|sr|fd|nbd|dm-) ]]; then
            continue
        fi

        if [[ "$disk_name" =~ ^(sd|hd|vd)[a-z]+$ ]] || [[ "$disk_name" =~ ^nvme[0-9]+n[0-9]+$ ]] || [[ "$disk_name" =~ ^mmcblk[0-9]+$ ]]; then
            local size_sectors=$(cat "$disk_path/size" 2>/dev/null || echo 0)
            local d_total=$((size_sectors * 512))
            if [ "$d_total" -gt 0 ]; then
                local d_used=0
                d_used=$(echo "$df_output" | awk -v disk="$disk_name" '
                    $1 ~ "^/dev/" disk "([0-9]|p[0-9]|$)" { used += $3 }
                    END { print used+0 }
                ')
                total_bytes=$((total_bytes + d_total))
                used_bytes=$((used_bytes + d_used))
            fi
        fi
    done

    if [ "$total_bytes" -gt 0 ]; then
        free_bytes=$((total_bytes - used_bytes))
        if [ "$free_bytes" -lt 0 ]; then free_bytes=0; fi
    else
        # Fallback para o sistema de arquivos raiz caso não encontre blocos em /sys
        if df_out=$(df -B1 / 2>/dev/null | awk 'END {print $2, $3, $4}'); then
            read -r total_bytes used_bytes free_bytes <<< "$df_out"
        fi
    fi

    echo "${total_bytes:-0} ${used_bytes:-0} ${free_bytes:-0}"
}

# ------------------------------------------------------------------------------
# Função de Auto-Discovery (Varredura de Rede via /api/autodiscovery)
# ------------------------------------------------------------------------------
auto_discovery_esp32() {
    local default_route base_ip i target response telemetry_target
    local config_file="${CONFIG_FILE}"

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
# Montagem do Payload no formato JSON com Dados Brutos e Detalhamento de Discos
# ------------------------------------------------------------------------------
build_telemetry_payload() {
    local id ip mac host uptime timestamp os_name os_logo kernel arch cpu_temp cpu_load mem_total_bytes mem_used_bytes mem_free_bytes disk_total_bytes disk_used_bytes disk_free_bytes os_info datetime epoch_timestamp hardware_discos_json

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
    hardware_discos_json=$(get_hardware_disk_metrics)

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
    },
    "hardware_discos": $hardware_discos_json
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
lucas@Servidor-LNX:~/telemetry$ c
lucas@Servidor-LNX:~/telemetry$ cat telemetry.sh
#!/usr/bin/env bash

# ==============================================================================
# Script: telemetry_deb_13.sh
# Descrição: Coletor de telemetria nativo em Bash adaptado ao novo schema JSON
#            com Auto-Discovery e verificação completa de discos físicos no SO.
# ==============================================================================

set -euo pipefail
LC_ALL=C # Força separador decimal padrão (ponto)

# ------------------------------------------------------------------------------
# Configurações Padrão & Variáveis de Ambiente
# ------------------------------------------------------------------------------
readonly DEFAULT_ENDPOINT="http://192.168.1.6/api/telemetry"
readonly DEFAULT_TIMEOUT=5
readonly DEFAULT_LOGO="https://cdn.iconscout.com/icon/free/png-256/free-debian-icon-svg-download-png-202378.png"
readonly DEFAULT_LOGO_UBUNTU="https://assets.ubuntu.com/v1/29383635-ubuntu-logo-2022.png"
readonly DEFAULT_LOGO_RASPBERRY="https://www.raspberrypi.com/app/uploads/2020/06/raspberrry_pi_logo.png"
readonly DEFAULT_SERVER_NAME="Servidor-LNX"

SERVER_NAME="${SERVER_NAME:-$DEFAULT_SERVER_NAME}"
ENDPOINT_URL="${ENDPOINT_URL:-$DEFAULT_ENDPOINT}"
TIMEOUT_SEC="${TIMEOUT_SEC:-$DEFAULT_TIMEOUT}"
OS_LOGO_URL="${OS_LOGO_URL:-}"


# ------------------------------------------------------------------------------
# Configuração Externa (Garante que o config.env exista)
# ------------------------------------------------------------------------------
CONFIG_DIR="${HOME}/telemetry"
CONFIG_FILE="${CONFIG_DIR}/config.env"
DEFAULT_FALLBACK_ENDPOINT="http://192.168.1.6/api/telemetry"

if [[ ! -f "$CONFIG_FILE" ]]; then
    mkdir -p "$CONFIG_DIR" 2>/dev/null || true
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
    ip=$(ip route get 1.1.1.1 2>/dev/null | awk '{print $7}' || hostname -I | awk '{print $1}')
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
    # Garante estritamente que qualquer vírgula seja convertida para ponto
    echo "${temp:-0.0}" | tr ',' '.'
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
            $1 ~ "^/dev/" disk "([0-9]|p[0-9]|$)" { used += $3 }
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

get_disk_metrics() {
    # Coleta valores brutos em Bytes agregando todos os discos físicos do sistema
    local total_bytes=0 used_bytes=0 free_bytes=0
    local df_output
    df_output=$(df -B1 2>/dev/null || true)

    for disk_path in /sys/block/*; do
        [ -e "$disk_path" ] || continue
        local disk_name=$(basename "$disk_path")

        if [[ "$disk_name" =~ ^(loop|ram|zram|sr|fd|nbd|dm-) ]]; then
            continue
        fi

        if [[ "$disk_name" =~ ^(sd|hd|vd)[a-z]+$ ]] || [[ "$disk_name" =~ ^nvme[0-9]+n[0-9]+$ ]] || [[ "$disk_name" =~ ^mmcblk[0-9]+$ ]]; then
            local size_sectors=$(cat "$disk_path/size" 2>/dev/null || echo 0)
            local d_total=$((size_sectors * 512))
            if [ "$d_total" -gt 0 ]; then
                local d_used=0
                d_used=$(echo "$df_output" | awk -v disk="$disk_name" '
                    $1 ~ "^/dev/" disk "([0-9]|p[0-9]|$)" { used += $3 }
                    END { print used+0 }
                ')
                total_bytes=$((total_bytes + d_total))
                used_bytes=$((used_bytes + d_used))
            fi
        fi
    done

    if [ "$total_bytes" -gt 0 ]; then
        free_bytes=$((total_bytes - used_bytes))
        if [ "$free_bytes" -lt 0 ]; then free_bytes=0; fi
    else
        # Fallback para o sistema de arquivos raiz caso não encontre blocos em /sys
        if df_out=$(df -B1 / 2>/dev/null | awk 'END {print $2, $3, $4}'); then
            read -r total_bytes used_bytes free_bytes <<< "$df_out"
        fi
    fi

    echo "${total_bytes:-0} ${used_bytes:-0} ${free_bytes:-0}"
}

# ------------------------------------------------------------------------------
# Função de Auto-Discovery (Varredura de Rede via /api/autodiscovery)
# ------------------------------------------------------------------------------
auto_discovery_esp32() {
    local default_route base_ip i target response telemetry_target
    local config_file="${CONFIG_FILE}"

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
# Montagem do Payload no formato JSON com Dados Brutos e Detalhamento de Discos
# ------------------------------------------------------------------------------
build_telemetry_payload() {
    local id ip mac host uptime timestamp os_name os_logo kernel arch cpu_temp cpu_load mem_total_bytes mem_used_bytes mem_free_bytes disk_total_bytes disk_used_bytes disk_free_bytes os_info datetime epoch_timestamp hardware_discos_json

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
    hardware_discos_json=$(get_hardware_disk_metrics)

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
    },
    "hardware_discos": $hardware_discos_json
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