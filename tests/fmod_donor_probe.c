#include "elf32_loader.h"
#include "compat_bridge.h"
#include "initializer_trace.h"
#include "relocation_probe.h"
#include "jni_bridge.h"
#include "fmod_filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

/* Isolated integration probe: loads the user's FMOD donor, never libapp.so. */
int main(int argc, char **argv)
{
    const char *names[] = { "libc++_shared.so", "libfmodex.so" };
    struct elf32_image images[2] = {0};
    struct nfsmw_relocation_probe_stats stats;
    char path[4096], error[512];
    size_t i, called = 0;
    void *system = NULL, *sound = NULL;
    int result, expected, failures = 0;
    int (*create)(void **);
    int (*init)(void *, int, unsigned int, void *);
    int (*output)(void *, int);
    int (*create_sound)(void *, const char *, unsigned int, void *, void **);
    int (*release_sound)(void *);
    int (*release_system)(void *);
    nfsmw_fmod_set_filesystem_fn set_fs;
    unsigned int modes[] = { 0xa0U };
    const char *assets[] = { "published/sounds/music/loading_01.mp3",
                            "published/sounds/ui/ui.fsb" };

    if (argc != 4) {
        fprintf(stderr, "Usage: %s LIBDIR AUDIO_ROOT EXPECTED_RESULT\n", argv[0]);
        return 2;
    }
    expected = atoi(argv[3]);
    setvbuf(stdout, NULL, _IONBF, 0U);
    setvbuf(stderr, NULL, _IONBF, 0U);
    setenv("NFSMW_AUDIO_ROOT", argv[2], 1);
    if (dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_GLOBAL) == NULL) {
        fprintf(stderr, "Cannot load ARM compiler helpers: %s\n", dlerror());
        return 2;
    }
    nfsmw_compat_init();
    for (i = 0; i < 2; ++i) {
        snprintf(path, sizeof(path), "%s/%s", argv[1], names[i]);
        if (elf32_map(&images[i], path, error, sizeof(error))) {
            fprintf(stderr, "%s\n", error); return 1;
        }
    }
    if (nfsmw_relocation_probe(images, 2, &stats, error, sizeof(error)) ||
        stats.unresolved_relocations) {
        fprintf(stderr, "Relocation error: %s\n", error); return 1;
    }
    if (nfsmw_apply_fmod_patches(&images[1], error, sizeof(error))) {
        fprintf(stderr, "%s\n", error); return 1;
    }
    for (i = 0; i < 2; ++i) {
        if (nfsmw_run_initializers(&images[i], &called, error, sizeof(error))) {
            fprintf(stderr, "%s\n", error); return 1;
        }
    }
#define LOOKUP(variable, symbol) do { \
    uintptr_t address = elf32_find_export(&images[1], symbol); \
    if (!address || sizeof(variable) != sizeof(address)) return 3; \
    memcpy(&(variable), &address, sizeof(variable)); \
} while (0)
    LOOKUP(create, "FMOD_System_Create");
    LOOKUP(init, "FMOD_System_Init");
    LOOKUP(output, "FMOD_System_SetOutput");
    LOOKUP(set_fs, "FMOD_System_SetFileSystem");
    LOOKUP(create_sound, "FMOD_System_CreateSound");
    LOOKUP(release_sound, "FMOD_Sound_Release");
    LOOKUP(release_system, "FMOD_System_Release");
    result = create(&system);
    printf("PROBE create=%d system=%p\n", result, system);
    if (result) return 4;
    result = output(system, 2); /* FMOD_OUTPUTTYPE_NOSOUND */
    printf("PROBE output=%d\n", result);
    if (result) return 5;
    result = nfsmw_fmod_install_filesystem(set_fs, system, -1);
    printf("PROBE filesystem=%d\n", result);
    if (result) return 5;
    result = init(system, 32, 0U, NULL);
    printf("PROBE init=%d\n", result);
    if (result) return 5;
    for (i = 0; i < sizeof(assets) / sizeof(assets[0]); ++i) {
        size_t m;
        for (m = 0; m < sizeof(modes) / sizeof(modes[0]); ++m) {
            sound = NULL;
            result = create_sound(system, assets[i], modes[m], NULL, &sound);
            printf("PROBE sound path=%s mode=%x result=%d handle=%p\n",
                   assets[i], modes[m], result, sound);
            if (result != expected || (result == 0 && sound == NULL)) ++failures;
            if (result == 0 && sound != NULL) release_sound(sound);
        }
    }
    release_system(system);
    return failures == 0 ? 0 : 6;
}
