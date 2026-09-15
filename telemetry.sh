#!/usr/bin/env bash

# ==============================================================================
# Script: telemetry.sh
# Descrição: Coletor de telemetry nativo em Bash adaptado ao novo schema JSON.
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
    local total_mb=0 used_mb=0 pct=0.00
    if [[ -f /proc/meminfo ]]; then
        local mem_total_kb mem_avail_kb mem_used_kb
        mem_total_kb=$(awk '/MemTotal:/ {print $2}' /proc/meminfo || echo "0")
        mem_avail_kb=$(awk '/MemAvailable:/ {print $2}' /proc/meminfo || echo "")

        if [[ -z "$mem_avail_kb" ]]; then
            local mem_free_kb mem_buffers_kb mem_cached_kb
            mem_free_kb=$(awk '/MemFree:/ {print $2}' /proc/meminfo || echo "0")
            mem_buffers_kb=$(awk '/Buffers:/ {print $2}' /proc/meminfo || echo "0")
            mem_cached_kb=$(awk '/^Cached:/ {print $2}' /proc/meminfo || echo "0")
            mem_avail_kb=$(( ${mem_free_kb:-0} + ${mem_buffers_kb:-0} + ${mem_cached_kb:-0} ))
        fi

        if [[ -n "$mem_total_kb" && "$mem_total_kb" -gt 0 ]]; then
            mem_used_kb=$(( mem_total_kb - mem_avail_kb ))
            total_mb=$(( mem_total_kb / 1024 ))
            used_mb=$(( mem_used_kb / 1024 ))
            pct=$(awk -v u="$mem_used_kb" -v t="$mem_total_kb" 'BEGIN { if (t>0) printf "%.2f", (u/t)*100; else print "0.00" }')
            echo "$total_mb $used_mb $pct"
            return
        fi
    fi

    local total used
    read -r total used < <(free -m 2>/dev/null | awk '/Mem:|Memória:/ {print $2, $3}')
    total="${total:-0}"
    used="${used:-0}"
    pct=$(awk -v u="$used" -v t="$total" 'BEGIN { if (t>0) printf "%.2f", (u/t)*100; else print "0.00" }')
    echo "$total $used $pct"
}

get_disk_metrics() {
    local total_gb=0.0 used_gb=0.0 pct=0
    if df_out=$(df -Pk / 2>/dev/null | awk 'END {print $2, $3, $5}'); then
        read -r total_kb used_kb pct_str <<< "$df_out"
        total_gb=$(awk -v t="${total_kb:-0}" 'BEGIN {printf "%.1f", t/1048576}')
        used_gb=$(awk -v u="${used_kb:-0}" 'BEGIN {printf "%.1f", u/1048576}')
        pct="${pct_str%%%}"
    fi
    echo "$total_gb $used_gb $pct"
}

get_disk_free() {
    local free_str
    free_str=$(df -h / | awk 'END {print $4}')
    echo "${free_str:-0B}"
}

# ------------------------------------------------------------------------------
# Montagem do Payload no novo formato JSON
# ------------------------------------------------------------------------------
build_telemetry_payload() {
    local id ip mac host uptime timestamp os_name os_logo kernel arch cpu_temp cpu_load mem_total_mb mem_used_mb mem_pct disk_total_gb disk_used_gb disk_pct disk_free os_info datetime epoch_timestamp

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

    read -r mem_total_mb mem_used_mb mem_pct <<< "$(get_memory_metrics)"
    read -r disk_total_gb disk_used_gb disk_pct <<< "$(get_disk_metrics)"
    disk_free=$(get_disk_free)

    # Fallbacks numéricos de segurança
    cpu_temp="${cpu_temp:-0.0}"
    cpu_load="${cpu_load:-0.0}"
    mem_total_mb="${mem_total_mb:-0}"
    mem_used_mb="${mem_used_mb:-0}"
    mem_pct="${mem_pct:-0.00}"
    disk_total_gb="${disk_total_gb:-0.0}"
    disk_used_gb="${disk_used_gb:-0.0}"
    disk_pct="${disk_pct:-0}"

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
      "total_mb": $mem_total_mb,
      "usada_mb": $mem_used_mb,
      "percentual": $mem_pct
    },
    "disco": {
      "total_gb": $disk_total_gb,
      "usado_gb": $disk_used_gb,
      "uso_percentual": $disk_pct,
      "espaco_livre": "$disk_free"
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
        exit 1
    fi
}

main "$@"