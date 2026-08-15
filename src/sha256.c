#include "hids.h"
#include <openssl/evp.h>

int sha256_file(const char *path, char hash[65])
{
    FILE          *fp;
    unsigned char  digest[EVP_MAX_MD_SIZE];
    unsigned int   digest_len = 0;
    unsigned char  buf[8192];
    EVP_MD_CTX    *ctx;
    const EVP_MD  *md;
    size_t         n;

    if (!path || !hash) {
        log_error(__FILE__, __LINE__, "sha256: NULL аргумент");
        return -1;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        log_error(__FILE__, __LINE__, "sha256: fopen(%s): %s", path, strerror(errno));
        return -1;
    }

    /* Создание контекста */
    ctx = EVP_MD_CTX_new();
    if (!ctx) {
        log_error(__FILE__, __LINE__, "sha256: EVP_MD_CTX_new: ошибка");
        fclose(fp);
        return -1;
    }

    /* Получение алгоритма SHA-256 */
    md = EVP_sha256();
    if (!md) {
        log_error(__FILE__, __LINE__, "sha256: EVP_sha256: ошибка");
        EVP_MD_CTX_free(ctx);
        fclose(fp);
        return -1;
    }

    /* Инициализация контекста */
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        log_error(__FILE__, __LINE__, "sha256: EVP_DigestInit_ex: ошибка");
        EVP_MD_CTX_free(ctx);
        fclose(fp);
        return -1;
    }

    /* Чтение файла и обновление хэша */
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (EVP_DigestUpdate(ctx, buf, n) != 1) {
            log_error(__FILE__, __LINE__, "sha256: EVP_DigestUpdate: ошибка");
            EVP_MD_CTX_free(ctx);
            fclose(fp);
            return -1;
        }
    }

    if (ferror(fp)) {
        log_error(__FILE__, __LINE__, "sha256: read(%s): %s", path, strerror(errno));
        EVP_MD_CTX_free(ctx);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    /* Получение итогового хэша */
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        log_error(__FILE__, __LINE__, "sha256: EVP_DigestFinal_ex: ошибка");
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    /* Освобождение контекста */
    EVP_MD_CTX_free(ctx);

    /* Преобразование бинарного хэша в hex-строку */
    for (unsigned int i = 0; i < digest_len; i++) {
        sprintf(hash + i * 2, "%02x", digest[i]);
    }
    hash[digest_len * 2] = '\0';

    return 0;
}