#include <string.h>
#include <stdio.h>          /* perror, printf                  */
#include <stdlib.h>         /* exit, EXIT_SUCCESS/FAILURE      */
#include <unistd.h>         /* fork, setsid, chdir, close      */
#include <fcntl.h>          /* open, O_RDWR                    */
#include <signal.h>         /* sigaction, SIGHUP, SIG_IGN      */
#include <sys/stat.h>       /* umask                           */
#include <sys/resource.h>   /* getrlimit, struct rlimit        */
#include <sys/inotify.h>    /* inotify_init, inotify_add_watch */
#include <stdint.h>         /* uint32_t                        */
#include <stdarg.h>         /* va_list, va_start, va_end       */
#include <dirent.h>         /* opendir, readdir, closedir      */
#include <syslog.h>
#include <errno.h>

/* Глобальные флаги сигналов (определены в signals.c) */
extern volatile sig_atomic_t g_running;
extern volatile sig_atomic_t g_reload_config;
extern volatile sig_atomic_t g_force_check;

/* Константы */
#define PID_FILE         "/tmp/hids.pid"
#define BASELINE_FILE    "/var/lib/hids/baseline.db"
#define MAX_PATH_LEN     4096
#define EVENT_BUF_SIZE   4096

/* logger.c */
void logger_init();
void logger_close(void);

void log_debug(const char *file, int line, const char *fmt, ...);
void log_info (const char *file, int line, const char *fmt, ...);
void log_warn (const char *file, int line, const char *fmt, ...);
void log_error(const char *file, int line, const char *fmt, ...);
void log_alert(const char *file, int line, const char *fmt, ...);
void log_fatal(const char *file, int line, const char *fmt, ...);

/* pidfile.c */
int create_pid_file();
void remove_pidfile();

/* monitor.c */
int  monitor_init(const char *path);
void monitor_run(int fd);
void monitor_close(int fd);

/* signals.c */
void signals_init(void);

/* sha256.c */
int sha256_file(const char *path, char hash[65]);

/* baseline.c */
int  baseline_load(const char *path);
int  baseline_save(const char *path);
void baseline_free(void);
int  baseline_check_file(const char *path);
int  baseline_scan_directory(const char *dir);