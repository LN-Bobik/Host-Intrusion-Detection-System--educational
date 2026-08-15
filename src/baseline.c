/*
    baseline.c - база хэшей для контроля целостности
    Хранение: текстовый файл /var/lib/hids/baseline.db
    Права доступа: 700 на каталог, 600 на файл
 */

#include "hids.h"

#define BASELINE_DIR  "/var/lib/hids"
#define BASELINE_FILE "/var/lib/hids/baseline.db"

/* Запись базы верных хэшей */
typedef struct baseline_entry {
    char    path[MAX_PATH_LEN];
    char    hash[65];
    off_t   size;
    time_t  mtime;
    struct baseline_entry *next;
} baseline_entry_t;

/* Голова списка */
static baseline_entry_t *baseline_head = NULL;

static int baseline_dir_create(void)
{
    struct stat st;
    if (stat(BASELINE_DIR, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            log_error(__FILE__, __LINE__, "baseline: %s не каталог", BASELINE_DIR);
            return -1;
        }
        /* Проверка прав: если слишком открыты - исправить */
        if (st.st_mode & 0077) {
            log_warn(__FILE__, __LINE__, "baseline: права на %s недопустимы, исправление...", BASELINE_DIR);
            chmod(BASELINE_DIR, 0700);
        }
        return 0;
    }

    /* Создание каталога */
    if (mkdir(BASELINE_DIR, 0700) < 0) {
        log_error(__FILE__, __LINE__, "baseline: mkdir(%s): %s", BASELINE_DIR, strerror(errno));
        return -1;
    }

    log_info(__FILE__, __LINE__, "baseline: создан каталог %s (0700)", BASELINE_DIR);
    return 0;
}

static baseline_entry_t *baseline_entry_create(const char *path, const char *hash, off_t size, time_t mtime)
{
    baseline_entry_t *entry = malloc(sizeof(*entry));
    if (!entry) {
        return NULL;
    }

    strncpy(entry->path, path, MAX_PATH_LEN - 1);
    entry->path[MAX_PATH_LEN - 1] = '\0';
    strncpy(entry->hash, hash, 64);
    entry->hash[64] = '\0';
    entry->size = size;
    entry->mtime = mtime;
    entry->next = NULL;
    return entry;
}

static void baseline_entry_free(baseline_entry_t *entry)
{
    if (entry) {
        free(entry);
    }
}


static void baseline_add(baseline_entry_t *entry)
{
    if (!baseline_head) {
        baseline_head = entry;
        return;
    }

    baseline_entry_t *cur = baseline_head;
    while (cur->next) {
        cur = cur->next;
    }
    cur->next = entry;
}


static baseline_entry_t *baseline_find(const char *path)
{
    for (baseline_entry_t *cur = baseline_head; cur; cur = cur->next) {
        if (strcmp(cur->path, path) == 0) {
            return cur;
        }
    }
    return NULL;
}

static FILE *baseline_file_open(const char *mode)
{
    FILE *fp;
    int   fd;

    /* Создание каталога (если нет) */
    if (baseline_dir_create() < 0) {
        return NULL;
    }

    /* Открытие с явными правами 600 для записи */
    if (strchr(mode, 'w')) {
        fd = open(BASELINE_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (fd < 0) {
            log_error(__FILE__, __LINE__, "baseline: open(%s): %s", BASELINE_FILE, strerror(errno));
            return NULL;
        }
        fp = fdopen(fd, mode);
    } else {
        fd = open(BASELINE_FILE, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            return NULL;
        }
        fp = fdopen(fd, mode);
    }

    if (!fp) {
        log_error(__FILE__, __LINE__, "baseline: fdopen: %s", strerror(errno));
        close(fd);
        return NULL;
    }

    /* Убедиться, что права 600 */
    fchmod(fileno(fp), 0600);

    return fp;
}

/* Загрузка базы из файла */
int baseline_load(const char *path)
{
    FILE *fp;
    char  line[1024];

    fp = fopen(path, "r");
    if (!fp) {
        log_warn(__FILE__, __LINE__, "baseline: %s не найден", path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *path_str, *hash_str, *size_str, *mtime_str;
        char *saveptr;

        /* Удаление символа перевода строки */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        /* Пропуск пустых строк и комментариев */
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Разбор строки: путь|sha256|размер|mtime */
        path_str = strtok_r(line, "|", &saveptr);
        hash_str = strtok_r(NULL, "|", &saveptr);
        size_str = strtok_r(NULL, "|", &saveptr);
        mtime_str = strtok_r(NULL, "|", &saveptr);

        if (!path_str || !hash_str || !size_str || !mtime_str) {
            log_warn(__FILE__, __LINE__, "baseline: некорректная строка: %s", line);
            continue;
        }

        off_t  size  = strtoll(size_str, NULL, 10);
        time_t mtime = strtoll(mtime_str, NULL, 10);

        baseline_entry_t *entry = baseline_entry_create(path_str, hash_str, size, mtime);
        if (!entry) {
            log_error(__FILE__, __LINE__, "baseline: не удалось выделить память");
            fclose(fp);
            return -1;
        }

        baseline_add(entry);
    }

    fclose(fp);
    log_info(__FILE__, __LINE__, "baseline: загружено из %s", path);
    return 0;
}

int baseline_save(const char *path)
{
    FILE *fp;

    /* Создание каталога */
    if (baseline_dir_create() < 0) {
        return -1;
    }

    /* Открытие с правами 600 */
    fp = baseline_file_open("w");
    if (!fp) {
        return -1;
    }
    fprintf(fp, "# HIDS baseline database\n");
    fprintf(fp, "# path|sha256|size|mtime\n");

    for (baseline_entry_t *cur = baseline_head; cur; cur = cur->next) {
        fprintf(fp, "%s|%s|%lld|%lld\n",
                cur->path, cur->hash,
                (long long)cur->size, (long long)cur->mtime);
    }

    fclose(fp);
    log_info(__FILE__, __LINE__, "baseline: сохранено в %s (0600)", path);
    return 0;
}

/* Освобождение всей базы из памяти. */
void baseline_free(void)
{
    baseline_entry_t *cur = baseline_head;
    while (cur) {
        baseline_entry_t *next = cur->next;
        baseline_entry_free(cur);
        cur = next;
    }
    baseline_head = NULL;
}

int baseline_check_file(const char *path)
{
    char   current_hash[65];
    struct stat st;

    /* Получение текущего состояния файла */
    if (stat(path, &st) < 0) {
        log_warn(__FILE__, __LINE__, "baseline: stat(%s): %s", path, strerror(errno));
        return -1;
    }

    /* Получение текущего хэша */
    if (sha256_file(path, current_hash) < 0) {
        return -1;
    }

    /* Поиск в базе */
    baseline_entry_t *entry = baseline_find(path);
    if (!entry) {
        log_alert(__FILE__, __LINE__, "baseline: новый файл не в базе: %s", path);
        return 1;
    }

    /* Сравнение */
    if (strcmp(entry->hash, current_hash) != 0) {
        log_alert(__FILE__, __LINE__,
                  "baseline: ИЗМЕНЁН %s\n  было: %s\n  стало: %s",
                  path, entry->hash, current_hash);
        return 1;
    }

    /* Обновление размера и mtime */
    entry->size = st.st_size;
    entry->mtime = st.st_mtime;

    return 0;
}

int baseline_scan_directory(const char *dir) // Рекурсивное сканирование директории
{
    DIR           *dp;
    struct dirent *entry;
    struct stat    st;
    char           fullpath[MAX_PATH_LEN];

    dp = opendir(dir);
    if (!dp) {
        log_error(__FILE__, __LINE__, "baseline: opendir(%s): %s", dir, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dp)) != NULL) {
        /* Пропуск . и .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);

        if (lstat(fullpath, &st) < 0) {
            continue;
        }

        /* Рекурсивный спуск в поддиректории */
        if (S_ISDIR(st.st_mode)) {
            baseline_scan_directory(fullpath);
            continue;
        }

        /* Только обычные файлы */
        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        /* Вычисление хэша */
        char hash[65];
        if (sha256_file(fullpath, hash) < 0) {
            continue;
        }

        /* Добавление в базу */
        baseline_entry_t *entry = baseline_entry_create(fullpath, hash, st.st_size, st.st_mtime);
        if (!entry) {
            closedir(dp);
            return -1;
        }

        baseline_add(entry);
        log_debug(__FILE__, __LINE__, "baseline: добавлен %s (%s)", fullpath, hash);
    }

    closedir(dp);
    return 0;
}