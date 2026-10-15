#include "Hours_Time.h"
#include "FileSystemControll.h"

Hours_Time* Hours_Time::_instance = nullptr;

Hours_Time::Hours_Time(const char* hours_sleep, const char* hours_wakeon, const char* date, long gmtOffset_sec, int daylightOffset_sec, const char* ntpServer)
    : hours_sleep(hours_sleep), hours_wakeon(hours_wakeon), date(date), gmtOffset_sec(gmtOffset_sec), daylightOffset_sec(daylightOffset_sec), ntpServer(ntpServer) {
    _brightness = &g_file_system_ctrl;
    _instance = this;
}

Hours_Time::~Hours_Time() {
    if (_instance == this) _instance = nullptr;
}

Hours_Time* Hours_Time::instance() { return _instance; }

void Hours_Time::setBrightnessControl(IBrightnessControl* brightness) { _brightness = brightness; }

const char* Hours_Time::getHoursWakeon() const {
    return hours_wakeon.c_str();
}

const char* Hours_Time::getHoursSleep() const {
    return hours_sleep.c_str();
}
void Hours_Time::setHoursSleep(const String& sleep_time) {
    if (!isValidTimeFormat(sleep_time.c_str())) return;
    this->hours_sleep = sleep_time;
    _night_state = -1; // Reavalia o período na próxima checagem
    //Serial.printf("[Hours_Time] Novo horário de sleep configurado: %s\n", this->hours_sleep.c_str());
}

void Hours_Time::setHoursWakeon(const String& wake_time) {
    if (!isValidTimeFormat(wake_time.c_str())) return;
    this->hours_wakeon = wake_time;
    _night_state = -1; // Reavalia o período na próxima checagem
    //Serial.printf("[Hours_Time] Novo horário de wakeon configurado: %s\n", this->hours_wakeon.c_str());
}

void Hours_Time::time_server() {
    // Configura o serviço de tempo NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("\nServiço NTP configurado. Aguardando a primeira sincronização...");
}

void Hours_Time::calendar() {
    // Executa a checagem apenas a cada 10 segundos
    static unsigned long lastCalendarCheck = 0;
    if (millis() - lastCalendarCheck < 10000) return;
    lastCalendarCheck = millis();

    // 1. Só tenta buscar o tempo se o Wi-Fi estiver conectado
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Aguardando conexão Wi-Fi para sincronizar relógio...");
        return;
    }

    // 2. Obtém a data e hora do relógio interno
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 100)) {
        Serial.println("Falha ao obter o tempo. Tentando novamente...");
        return;
    }
    
    // Imprime os detalhes no Monitor Serial
    Serial.println("--- Tempo Atual ---");
    char timeString[50];
    strftime(timeString, sizeof(timeString), "%d/%m/%Y %H:%M:%S", &timeinfo);
    Serial.printf("Data e Hora: %s\n", timeString);

    char dayOfWeek[10];
    strftime(dayOfWeek, sizeof(dayOfWeek), "%A", &timeinfo);
    Serial.printf("Dia da Semana: %s\n", dayOfWeek);
    Serial.println("-------------------");
}

void Hours_Time::onSleep(std::function<void()> callback) { _onSleep = callback; }

void Hours_Time::onWake(std::function<void()> callback) { _onWake = callback; }

void Hours_Time::notificar(bool dormir) {
    // Ação padrão: apaga o backlight no período noturno e restaura o brilho anterior ao acordar
    if (_brightness != nullptr) {
        if (dormir) {
            uint8_t atual = _brightness->obterBrilhoPorcentagem();
            if (atual > 0) _brilho_antes_de_dormir = atual;
            _brightness->setBrilhoPorcentagem(0);
            _display_apagado = true;
        } else if (_display_apagado) {
            // Só restaura se este módulo apagou o display (no boot diurno o brilho não é alterado)
            _brightness->setBrilhoPorcentagem(_brilho_antes_de_dormir);
            _display_apagado = false;
        }
    }

    if (dormir) { if (_onSleep) _onSleep(); }
    else        { if (_onWake) _onWake(); }
}

bool Hours_Time::isValidTimeFormat(const char* hhmm) {
    if (hhmm == nullptr || strlen(hhmm) != 5 || hhmm[2] != ':') return false;
    if (!isdigit((unsigned char)hhmm[0]) || !isdigit((unsigned char)hhmm[1])) return false;
    if (!isdigit((unsigned char)hhmm[3]) || !isdigit((unsigned char)hhmm[4])) return false;
    int h = (hhmm[0] - '0') * 10 + (hhmm[1] - '0');
    int m = (hhmm[3] - '0') * 10 + (hhmm[4] - '0');
    return h <= 23 && m <= 59;
}

bool Hours_Time::isNightPeriod(const char* hhmm) const {
    int ordem = strncmp(hours_sleep.c_str(), hours_wakeon.c_str(), 5);
    if (ordem == 0) return false; // Janela vazia

    bool depoisDeDormir = strncmp(hhmm, hours_sleep.c_str(), 5) >= 0;
    bool antesDeAcordar = strncmp(hhmm, hours_wakeon.c_str(), 5) < 0;

    // sleep > wakeon: o período noturno atravessa a meia-noite
    return (ordem > 0) ? (depoisDeDormir || antesDeAcordar) : (depoisDeDormir && antesDeAcordar);
}

void Hours_Time::weke_on() {
    _instance = this; // Garante o registro mesmo se o objeto global tiver sido copiado na construção

    // Checagem 1 vez por segundo
    static unsigned long lastTimeCheck = 0;
    if (millis() - lastTimeCheck < 1000) return;
    lastTimeCheck = millis();

    struct tm timeinfo; 
    if (!getLocalTime(&timeinfo, 0)) return; // tmo = 0: não bloqueia o loop enquanto o NTP não sincroniza

    char currentTimeStr[6]; 
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeinfo);

    // -------------------------------------------------------------------------
    // CONTROLE DE TIMEOUT (MODO MANUAL)
    // -------------------------------------------------------------------------
    if (is_manual_mode) {
        if (millis() - manual_on_timestamp >= TIMEOUT_MS) {
            is_manual_mode = false; // Sai do modo manual ao atingir o timeout
            Serial.println("Timeout de 5 minutos atingido. Desligando modo manual do display.");
            if (isNightPeriod(currentTimeStr)) notificar(true); // Volta a dormir
        }
        return; 
    }

    // -------------------------------------------------------------------------
    // CONTROLE AUTOMÁTICO DE HORÁRIO (SLEEP / WAKEON)
    // -------------------------------------------------------------------------
    static int ultimoMinutoExecutado = -1;

    if (timeinfo.tm_min != ultimoMinutoExecutado) {
        if (strncmp(currentTimeStr, hours_sleep.c_str(), 5) == 0) {
            Serial.println("Hora de Dormir atingida!");
            ultimoMinutoExecutado = timeinfo.tm_min;
        } 
        else if (strncmp(currentTimeStr, hours_wakeon.c_str(), 5) == 0) {
            Serial.println("Hora de Ligar atingida!");
            ultimoMinutoExecutado = timeinfo.tm_min;
        }
    }

    // Transição de período (também cobre boot/alteração de horários dentro do período noturno)
    int estadoAtual = isNightPeriod(currentTimeStr) ? 1 : 0;
    if (estadoAtual != _night_state) {
        _night_state = estadoAtual;
        notificar(estadoAtual == 1);
    }
}

void Hours_Time::manual_turn_on() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char currentTimeStr[6];
        strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeinfo); 
        
        // Verifica se estamos no período noturno (entre hours_sleep e hours_wakeon)
        if (isNightPeriod(currentTimeStr)) {    
            is_manual_mode = true;
            manual_on_timestamp = millis();
            notificar(false); // Acende o display durante o modo manual
            Serial.println("Display ligado manualmente (Modo Timeout ativado).");
        }
    }
}

void Hours_Time::monitorarConexao() {
    if (WiFi.status() != WL_CONNECTED) Serial.println("[NTP] Conexão Wi-Fi perdida. Aguardando reconexão...");
}

const char* Hours_Time::losttime() const {
    struct tm timeinfo;
    static char timeString[50];
    if (getLocalTime(&timeinfo)) {
        strftime(timeString, sizeof(timeString), "%d/%m/%Y %H:%M:%S", &timeinfo);
        return timeString;
    }
    return "00/00/0000 00:00:00"; // Retorno de segurança
}