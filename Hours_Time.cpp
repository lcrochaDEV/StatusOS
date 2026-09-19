#include "Hours_Time.h"

Hours_Time::Hours_Time(const char* hours_sleep, const char* hours_wakeon, const char* date, long gmtOffset_sec, int daylightOffset_sec, const char* ntpServer)
    : hours_sleep(hours_sleep), hours_wakeon(hours_wakeon), date(date), gmtOffset_sec(gmtOffset_sec), daylightOffset_sec(daylightOffset_sec), ntpServer(ntpServer) {
}

const char* Hours_Time::getHoursWakeon() const {
    return hours_wakeon.c_str();
}

const char* Hours_Time::getHoursSleep() const {
    return hours_sleep.c_str();
}
void Hours_Time::setHoursSleep(const String& sleep_time) {
    this->hours_sleep = sleep_time;
    //Serial.printf("[Hours_Time] Novo horário de sleep configurado: %s\n", this->hours_sleep.c_str());
}

void Hours_Time::setHoursWakeon(const String& wake_time) {
    this->hours_wakeon = wake_time;
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

void Hours_Time::weke_on() {
    // Checagem 1 vez por segundo
    static unsigned long lastTimeCheck = 0;
    if (millis() - lastTimeCheck < 1000) return;
    lastTimeCheck = millis();

    struct tm timeinfo; 
    if (!getLocalTime(&timeinfo)) return;

    char currentTimeStr[6]; 
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeinfo);

    // -------------------------------------------------------------------------
    // CONTROLE DE TIMEOUT (MODO MANUAL)
    // -------------------------------------------------------------------------
    if (is_manual_mode) {
        if (millis() - manual_on_timestamp >= TIMEOUT_MS) {
            is_manual_mode = false; // Sai do modo manual ao atingir o timeout
            Serial.println("Timeout de 5 minutos atingido. Desligando modo manual do display.");
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
}

void Hours_Time::manual_turn_on() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char currentTimeStr[6];
        strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeinfo); 
        
        // Verifica se estamos no período noturno (entre hours_sleep e hours_wakeon)
        if (strncmp(currentTimeStr, hours_sleep.c_str(), 5) >= 0 || strncmp(currentTimeStr, hours_wakeon.c_str(), 5) < 0) {    
            is_manual_mode = true;
            manual_on_timestamp = millis();
            Serial.println("Display ligado manualmente (Modo Timeout ativado).");
        }
    }
}

void Hours_Time::monitorarConexao() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Conexão Wi-Fi perdida. Aguardando reconexão...");
    }
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