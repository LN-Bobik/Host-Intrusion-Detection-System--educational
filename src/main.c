#include "hids.h"

void demonize()
{

    int                 fd;
    pid_t               pid;
    struct sigaction    sa;
    struct rlimit       rl;

    /* Сброс umask, для самостоятельного назначения прав демоном*/
    umask(0);

    if((pid = fork()) < 0){
        perror("fork");
        exit(1);
    }
    else if(pid != 0)
        exit(0);
    setsid();            // Перехват лидерства

    /* Страховка от обретения терминала в будущем */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0){
        perror("sigaction");
        exit(1);
    }
    /* Второй fork - гарантия, что процесс никогда не получит терминал */
    if((pid = fork()) < 0){
        perror("fork");
        exit(1);
    }
    else if(pid != 0)
        exit(0);
    
    /* Назначение корневого каталога текущем */
    if (chdir("/") < 0){
        perror("chdir");
        exit(1);
    }

    /* Перенаправить stdin, stdout, stderr в /dev/null */
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        perror("open /dev/null");
        exit(EXIT_FAILURE);
    }
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > 2) {
        close(fd);
}
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getrlimit");
        rl.rlim_max = 1024;
    }
    
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    
    /* Закрыть все, кроме 0,1,2 */
    for(rlim_t i = 3; i < rl.rlim_max; i++) {
        close(i);  // Игнорирование EBADF
    }
}

int main(){
    demonize();
    logger_init();
    signals_init();

int pid_fd = create_pid_file();
if (pid_fd < 0) {
    log_error(__FILE__, __LINE__, "main: демон уже запущен, выход");
    logger_close();
    exit(1);
}

/* Загрузка baseline */
if (baseline_load(BASELINE_FILE) < 0) {
    log_warn(__FILE__, __LINE__, "main: baseline не найден, создание файла хэшей...");

    baseline_scan_directory("/etc");
    baseline_save(BASELINE_FILE);
}

int fd = monitor_init("/etc");
if (fd < 0) {
    log_error(__FILE__, __LINE__, "main: ошибка мониторинга");
    remove_pidfile();
    logger_close();
    exit(1);
    }
    log_info(__FILE__, __LINE__, "main: демон запущен");
    monitor_run(fd);
    log_info(__FILE__, __LINE__, "main: демон завершает работу");

    monitor_close(fd);

    /* Сохранение baseline */
    baseline_save(BASELINE_FILE);
    baseline_free();

    remove_pidfile();
    logger_close();
return 0;
}