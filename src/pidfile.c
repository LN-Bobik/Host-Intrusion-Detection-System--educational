#include "hids.h"
#define PID_FILE "/tmp/hids.pid"
static int pidfile_fd = -1;             // Удержание fd в открытом виде для блокировки

int create_pid_file(void)
{
    int   fd;
    char  pid_str[16];
    struct flock lock;

    /* Открытие с блокировочными флагами */
    fd = open(PID_FILE, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0) {
        log_error(__FILE__, __LINE__, "pidfile: open(%s): %s", PID_FILE, strerror(errno));
        return -1;
    }

    /* Попытка захвата эксклюзивной блокировки */
    lock.l_type   = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start  = 0;
    lock.l_len    = 0;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            char buf[16] = {0};
            lseek(fd, 0, SEEK_SET);
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                char *nl = strchr(buf, '\n');
                if (nl) *nl = '\0';
                log_error(__FILE__, __LINE__, "pidfile: демон уже запущен (PID=%s)", buf);
            } else {
                log_error(__FILE__, __LINE__, "pidfile: демон уже запущен");
            }
        } else {
            log_error(__FILE__, __LINE__, "pidfile: fcntl lock: %s", strerror(errno));
        }
        close(fd);
        return -1;
    }

    /* Попытка блокировки - успешна, записываем свой PID */
    ftruncate(fd, 0);
    snprintf(pid_str, sizeof(pid_str), "%ld\n", (long)getpid());

    if (write(fd, pid_str, strlen(pid_str)) < 0) {
        log_error(__FILE__, __LINE__, "pidfile: write: %s", strerror(errno));
        close(fd);
        return -1;
    }

    /* Не закрываем fd - блокировка живёт, пока fd открыт */
    pidfile_fd = fd;

    log_info(__FILE__, __LINE__, "pidfile created: %s (pid=%ld)", PID_FILE, (long)getpid());
    return 0;
}

void remove_pidfile(void)
{
{
    if (pidfile_fd >= 0) {
        /* Явное снятие блокировки */
        struct flock lock;
        lock.l_type   = F_UNLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start  = 0;
        lock.l_len    = 0;
        lock.l_pid    = 0;

        fcntl(pidfile_fd, F_SETLK, &lock);
        close(pidfile_fd);
        pidfile_fd = -1;
    }

    if (unlink(PID_FILE) < 0) {
        log_error(__FILE__, __LINE__, "pidfile: unlink(%s): %s", PID_FILE, strerror(errno));
    } else {
        log_info(__FILE__, __LINE__, "pidfile removed");
    }
}
}