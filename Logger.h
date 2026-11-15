#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void log(const char* message);
    void log(const String& message);
    void logf(const char* format, ...);
    void vlogf(const char* format, va_list args);

    void setEnabled(bool enable);
    bool isEnabled() const;

private:
    Logger() : _logsEnabled(false), _firstLogAfterPrompt(true) {}
    bool _logsEnabled;
    bool _firstLogAfterPrompt;
};

#endif
