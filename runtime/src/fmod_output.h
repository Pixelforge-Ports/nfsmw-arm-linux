#ifndef NFSMW_FMOD_OUTPUT_H
#define NFSMW_FMOD_OUTPUT_H

#include <stdio.h>
#include <string.h>

typedef int (*nfsmw_fmod_set_output_fn)(void *, int);

static inline int nfsmw_fmod_output_symbol(const char *name)
{
    if (strcmp(name, "FMOD_System_SetOutput") == 0) return 0;
    if (strcmp(name, "_ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE") == 0)
        return 1;
    return -1;
}

static int nfsmw_fmod_select_output(nfsmw_fmod_set_output_fn set_output,
                                   void *system, int output)
{
    int result = set_output(system, output);
    (void)printf("G8-FMOD setOutput requested=%d result=%d\n", output, result);
    if (result != 0 && output != 0) {
        /* AUTODETECT lets this version of the guest library select a backend. */
        result = set_output(system, 0);
        (void)printf("G8-FMOD setOutput fallback=AUTODETECT result=%d\n", result);
    }
    return result;
}

#endif
