#include "Logger.h"

void Logger::log(const char* message) {
    if (!_logsEnabled || message == nullptr) return;

    if (_firstLogAfterPrompt) {
        Serial.print("\n[LOG] ");
        Serial.print(message);
        _firstLogAfterPrompt = false;
    } else {
        Serial.print("[LOG] ");
        Serial.print(message);
    }
}

void Logger::log(const String& message) {
    log(message.c_str());
}

void Logger::vlogf(const char* format, va_list args) {
    if (!_logsEnabled || format == nullptr) return;

    char buffer[128];
    va_list argsCopy;
    va_copy(argsCopy, args);
    int needed = vsnprintf(buffer, sizeof(buffer), format, args);

    if (needed >= (int)sizeof(buffer)) {
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

void Logger::logf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    this->vlogf(format, args);
    va_end(args);
}

void Logger::setEnabled(bool enable) {
    _logsEnabled = enable;
}

bool Logger::isEnabled() const {
    return _logsEnabled;
}
