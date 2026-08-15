/*
    Параметры openlog:
    ident:      имя программы("hids")
    option:     LOG_PID      добавлять PID в сообщения
                LOG_CONS     если syslogd недоступен - писать на консоль
                LOG_NDELAY   открыть соединение сразу, не откладывая
    facility:   LOG_DAEMON   категория "системный демон"
 */

#include "hids.h"

void logger_init()
{
    openlog("hids", LOG_PID | LOG_CONS | LOG_NDELAY | LOG_PERROR, LOG_DAEMON);
}

void logger_close(){
    closelog();
}

/* Отладочное сообщение */
void log_debug(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_DEBUG, fmt, ap);
    va_end(ap);
    (void)file;
    (void)line;
}

/* Информационное сообщение*/
void log_info(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_INFO, fmt, ap);
    va_end(ap);
    (void)file;
    (void)line;
}

/* Предупреждение*/
void log_warn(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_WARNING, fmt, ap);
    va_end(ap);
    (void)file;
    (void)line;
}

/* Ошибка */
void log_error(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
    (void)file;
    (void)line;
}

/* Инцидент безопасности */
void log_alert(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_ALERT, fmt, ap);
    va_end(ap);
    (void)file;
    (void)line;
}

/* Критическая ошибка, завершение демона */
void log_fatal(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_CRIT, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
    (void)file;
    (void)line;
}