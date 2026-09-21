#include "fmod_filesystem.h"
#include <assert.h>
#include <unistd.h>

static int check_callbacks(
    void *system, nfsmw_fmod_file_open_fn open_file,
    nfsmw_fmod_file_close_fn close_file, nfsmw_fmod_file_read_fn read_file,
    nfsmw_fmod_file_seek_fn seek_file,
    nfsmw_fmod_file_async_read_fn async_read,
    nfsmw_fmod_file_async_cancel_fn async_cancel, int alignment)
{
    const char *paths[] = {
        "/published/sounds/music/loading_01.mp3",
        "published/sounds/ui/ui.fsb",
        "sounds/ui/ui.fsb",
        "/sounds/music/loading_01.mp3"
    };
    size_t i;
    (void)system;
    (void)alignment;
    assert(async_read == NULL && async_cancel == NULL);
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        void *handle = NULL, *userdata = NULL;
        unsigned int size = 0, count = 0;
        char data[4];
        assert(open_file(paths[i], 0, &size, &handle, &userdata) == 0);
        assert(size == 8);
        assert(read_file(handle, data, sizeof(data), &count, userdata) == 0);
        assert(count == 4 && memcmp(data, "abcdefgh", 4) == 0);
        assert(seek_file(handle, 4, userdata) == 0);
        assert(read_file(handle, data, sizeof(data), &count, userdata) == 0);
        assert(count == 4 && memcmp(data, "efgh", 4) == 0);
        assert(read_file(handle, data, sizeof(data), &count, userdata) == 22);
        assert(count == 0);
        assert(seek_file(handle, 6, userdata) == 0);
        assert(read_file(handle, data, sizeof(data), &count, userdata) == 22);
        assert(count == 2 && memcmp(data, "gh", 2) == 0);
        assert(seek_file(handle, 0, userdata) == 0);
        assert(read_file(handle, data, sizeof(data), &count, userdata) == 0);
        assert(count == 4 && memcmp(data, "abcd", 4) == 0);
        assert(read_file(handle, NULL, 0, &count, userdata) == 0 && count == 0);
        assert(close_file(handle, userdata) == 0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    char path[4096];
    assert(argc == 2);
    assert(setenv("NFSMW_AUDIO_ROOT", argv[1], 1) == 0);
    assert(nfsmw_fmod_audio_path("/published/sounds/music/loading_01.mp3",
                                path, sizeof(path)) == 0);
    assert(nfsmw_fmod_audio_path("published/sounds/ui/ui.fsb",
                                path, sizeof(path)) == 0);
    assert(nfsmw_fmod_audio_path("published/sounds/../secret",
                                path, sizeof(path)) != 0);
    assert(nfsmw_fmod_audio_path("published/sounds-other/file",
                                path, sizeof(path)) != 0);
    assert(nfsmw_fmod_audio_path("sounds/../secret", path, sizeof(path)) != 0);
    assert(nfsmw_fmod_audio_path("published/sounds/ui/ui.fsb", path, 4) != 0);
    assert(nfsmw_fmod_install_filesystem(check_callbacks, NULL, 0) == 0);
    {
        void *handle = (void *)1, *userdata = (void *)1;
        unsigned int size = 999;
        assert(nfsmw_fmod_file_open("sounds/missing.fsb", 0, &size,
                                    &handle, &userdata) == 23);
        assert(handle == NULL && userdata == NULL && size == 0);
        assert(nfsmw_fmod_file_open(NULL, 0, &size, &handle, &userdata) == 37);
    }
    puts("PASS: FMOD audio paths, open, read, seek and close");
    return 0;
}
