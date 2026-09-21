#ifndef NFSMW_FMOD_FILESYSTEM_H
#define NFSMW_FMOD_FILESYSTEM_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <stdatomic.h>

/* Values from the supported game's FMOD Ex error table. */
enum {
    NFSMW_FMOD_OK = 0,
    NFSMW_FMOD_ERR_FILE_BAD = 19,
    NFSMW_FMOD_ERR_FILE_COULDNOTSEEK = 20,
    NFSMW_FMOD_ERR_FILE_EOF = 22,
    NFSMW_FMOD_ERR_FILE_NOTFOUND = 23,
    NFSMW_FMOD_ERR_INVALID_PARAM = 37
};

static void nfsmw_fmod_log_open(const char *name, const char *path,
                                int result, int system_error)
{
    static atomic_uint successes;
    static atomic_uint failures;
    atomic_uint *counter = result == 0 ? &successes : &failures;
    unsigned int count = atomic_load(counter);

    while (count < 12U) {
        if (atomic_compare_exchange_weak(counter, &count, count + 1U)) {
            (void)printf("G8-FMOD open result=%d errno=%d request=%.240s path=%.360s\n",
                         result, system_error, name != NULL ? name : "(null)",
                         path != NULL ? path : "(rejected)");
            break;
        }
    }
}

typedef int (*nfsmw_fmod_file_open_fn)(const char *, int, unsigned int *,
                                       void **, void **);
typedef int (*nfsmw_fmod_file_close_fn)(void *, void *);
typedef int (*nfsmw_fmod_file_read_fn)(void *, void *, unsigned int,
                                       unsigned int *, void *);
typedef int (*nfsmw_fmod_file_seek_fn)(void *, unsigned int, void *);
typedef int (*nfsmw_fmod_file_async_read_fn)(void *, void *);
typedef int (*nfsmw_fmod_file_async_cancel_fn)(void *, void *);
typedef int (*nfsmw_fmod_set_filesystem_fn)(
    void *, nfsmw_fmod_file_open_fn, nfsmw_fmod_file_close_fn,
    nfsmw_fmod_file_read_fn, nfsmw_fmod_file_seek_fn,
    nfsmw_fmod_file_async_read_fn, nfsmw_fmod_file_async_cancel_fn, int);

static int nfsmw_fmod_audio_path(const char *name, char *path, size_t size)
{
    const char *root = getenv("NFSMW_AUDIO_ROOT");
    const char *relative;
    int written;

    if (root == NULL || root[0] == '\0' || name == NULL ||
        strstr(name, "..") != NULL) return -1;
    while (*name == '/') ++name;
    if (strncmp(name, "published/sounds/", sizeof("published/sounds/") - 1U) == 0)
        relative = name + sizeof("published/") - 1U;
    else if (strncmp(name, "sounds/", sizeof("sounds/") - 1U) == 0)
        relative = name;
    else return -1;
    written = snprintf(path, size, "%s/published/%s", root, relative);
    return written >= 0 && (size_t)written < size ? 0 : -1;
}

static int nfsmw_fmod_file_open(const char *name, int unicode,
                                unsigned int *size, void **handle,
                                void **userdata)
{
    char path[4096];
    FILE *file;
    off_t length;

    if (size == NULL || handle == NULL || name == NULL || unicode != 0)
        return NFSMW_FMOD_ERR_INVALID_PARAM;
    *size = 0U;
    *handle = NULL;
    if (userdata != NULL) *userdata = NULL;
    if (nfsmw_fmod_audio_path(name, path, sizeof(path)) != 0) {
        nfsmw_fmod_log_open(name, NULL, NFSMW_FMOD_ERR_FILE_NOTFOUND, 0);
        return NFSMW_FMOD_ERR_FILE_NOTFOUND;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        int saved_errno = errno;
        int result = saved_errno == ENOENT ? NFSMW_FMOD_ERR_FILE_NOTFOUND :
                                             NFSMW_FMOD_ERR_FILE_BAD;
        nfsmw_fmod_log_open(name, path, result, saved_errno);
        return result;
    }
    if (fseeko(file, 0, SEEK_END) != 0 ||
        (length = ftello(file)) < 0 || (uintmax_t)length > UINT_MAX ||
        fseeko(file, 0, SEEK_SET) != 0) {
        int saved_errno = errno;
        (void)fclose(file);
        nfsmw_fmod_log_open(name, path, NFSMW_FMOD_ERR_FILE_BAD, saved_errno);
        return NFSMW_FMOD_ERR_FILE_BAD;
    }
    *size = (unsigned int)length;
    *handle = file;
    nfsmw_fmod_log_open(name, path, NFSMW_FMOD_OK, 0);
    return NFSMW_FMOD_OK;
}

static int nfsmw_fmod_file_close(void *handle, void *userdata)
{
    (void)userdata;
    if (handle == NULL) return NFSMW_FMOD_ERR_INVALID_PARAM;
    return fclose((FILE *)handle) == 0 ? NFSMW_FMOD_OK : NFSMW_FMOD_ERR_FILE_BAD;
}

static int nfsmw_fmod_file_read(void *handle, void *buffer,
                                unsigned int bytes, unsigned int *read,
                                void *userdata)
{
    size_t result;

    (void)userdata;
    if (read == NULL) return NFSMW_FMOD_ERR_INVALID_PARAM;
    *read = 0U;
    if (handle == NULL || (buffer == NULL && bytes != 0U))
        return NFSMW_FMOD_ERR_INVALID_PARAM;
    if (bytes == 0U) return NFSMW_FMOD_OK;
    result = fread(buffer, 1U, bytes, (FILE *)handle);
    *read = (unsigned int)result;
    if (ferror((FILE *)handle) != 0) return NFSMW_FMOD_ERR_FILE_BAD;
    return result == bytes ? NFSMW_FMOD_OK : NFSMW_FMOD_ERR_FILE_EOF;
}

static int nfsmw_fmod_file_seek(void *handle, unsigned int position,
                                void *userdata)
{
    (void)userdata;
    if (handle == NULL) return NFSMW_FMOD_ERR_INVALID_PARAM;
    return fseeko((FILE *)handle, (off_t)position,
                                    SEEK_SET) == 0 ? NFSMW_FMOD_OK :
           NFSMW_FMOD_ERR_FILE_COULDNOTSEEK;
}

static int nfsmw_fmod_install_filesystem(nfsmw_fmod_set_filesystem_fn set_fs,
                                         void *system, int block_align)
{
    int result = set_fs(system, nfsmw_fmod_file_open, nfsmw_fmod_file_close,
                        nfsmw_fmod_file_read, nfsmw_fmod_file_seek, NULL,
                        NULL, block_align);
    (void)printf("G8-FMOD native sound filesystem result=%d\n", result);
    return result;
}

#endif
