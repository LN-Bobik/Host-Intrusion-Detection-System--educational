#include "hids.h"
#define EVENT_BUF_SIZE  4096
static char watch_path[MAX_PATH_LEN]; // Отслеживаемый путь
int monitor_init(const char *path)
{
    int fd = inotify_init1(IN_CLOEXEC);
    if (fd < 0) {
        log_error(__FILE__, __LINE__, "inotify_init: %s", strerror(errno));
        return -1;
    }

    /*
    IN_MODIFY:       содержимое изменилось
    IN_CLOSE_WRITE:  файл закрыт после записи (главный триггер)
    IN_CREATE:       новый файл
    IN_DELETE:       файл удалён
    IN_MOVED_FROM:   переименован/перемещён отсюда
    IN_MOVED_TO:     переименован/перемещён сюда
    IN_ATTRIB:       изменены права/владелец
     */
    uint32_t mask = IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB;

    int wd = inotify_add_watch(fd, path, mask);
    if (wd < 0) {
        log_error(__FILE__, __LINE__, "inotify_add_watch(%s): %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    strncpy(watch_path, path, MAX_PATH_LEN - 1);
    watch_path[MAX_PATH_LEN - 1] = '\0';
    log_info(__FILE__, __LINE__, "мониторинг: %s (wd=%d, fd=%d)", path, wd, fd);
    return fd;
}

void monitor_run(int fd)
{
    char buf[EVENT_BUF_SIZE] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (g_running) {

        ssize_t n = read(fd, buf, sizeof(buf));

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            log_error(__FILE__, __LINE__, "read inotify: %s", strerror(errno));
            break;
        }
        for (char *p = buf; p < buf + n; ) {
            struct inotify_event *event = (struct inotify_event *)p;
            const char *name = (event->len > 0) ? event->name : "";
            /* Фильтрация шума */
            if (event->len > 0) {
                if (name[0] == '.') {
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                if (strstr(name, ".swp")) {
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                if (strstr(name, ".tmp")) {
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                if (name[strlen(name) - 1] == '~') {
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
            }
            /* Определение типа события */
            const char *type = "???";
            if (event->mask & IN_CREATE)      type = "создан";
            if (event->mask & IN_DELETE)      type = "удалён";
            if (event->mask & IN_MODIFY)      type = "изменён";
            if (event->mask & IN_CLOSE_WRITE) type = "закрыт после записи";
            if (event->mask & IN_MOVED_FROM)  type = "перемещён отсюда";
            if (event->mask & IN_MOVED_TO)    type = "перемещён сюда";
            if (event->mask & IN_ATTRIB)      type = "изменены атрибуты";
            if (event->mask & IN_ISDIR)       type = "(директория)";
            log_debug(__FILE__, __LINE__, "событие: %-20s  имя: %s", type, name);

            /* Проверка целостности */
            if ((event->mask & IN_CLOSE_WRITE || event->mask & IN_MOVED_TO) && event->len > 0) {
                size_t path_len = strlen(watch_path) + 1 + strlen(name) + 1;
                char *fullpath = malloc(path_len);
                if (!fullpath) {
                    log_error(__FILE__, __LINE__, "malloc: %s", strerror(errno));
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                snprintf(fullpath, path_len, "%s/%s", watch_path, name);

                struct stat st;
                if (stat(fullpath, &st) < 0) {
                    log_warn(__FILE__, __LINE__, "файл недоступен: %s", fullpath);
                    free(fullpath);
                    p += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                int result = baseline_check_file(fullpath);
                if (result > 0) {
                    log_alert(__FILE__, __LINE__, "нарушение целостности: %s", fullpath);
                }
                free(fullpath);
            }
            /* Переход к следующему событию */
            p += sizeof(struct inotify_event) + event->len;
        }
    }
}

void monitor_close(int fd)
{
    if (fd >= 0) {
        close(fd);
        log_info(__FILE__, __LINE__, "inotify закрыт");
    }
}