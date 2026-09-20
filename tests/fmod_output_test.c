#include "fmod_output.h"
#include <assert.h>

static int calls, requested_result, automatic_result;
static int fake_system;
static int select_output(void *system, int output)
{
    assert(system == &fake_system);
    ++calls;
    return output == 0 ? automatic_result : requested_result;
}

int main(void)
{
    assert(nfsmw_fmod_output_symbol("FMOD_System_SetOutput") == 0);
    assert(nfsmw_fmod_output_symbol(
        "_ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE") == 1);
    assert(nfsmw_fmod_output_symbol("FMOD_System_Init") == -1);
    requested_result = 0;
    assert(nfsmw_fmod_select_output(select_output, &fake_system, 18) == 0);
    assert(calls == 1);
    calls = 0;
    requested_result = 42;
    automatic_result = 0;
    assert(nfsmw_fmod_select_output(select_output, &fake_system, 18) == 0);
    assert(calls == 2);
    calls = 0;
    automatic_result = 43;
    assert(nfsmw_fmod_select_output(select_output, &fake_system, 18) == 43);
    assert(calls == 2);
    calls = 0;
    assert(nfsmw_fmod_select_output(select_output, &fake_system, 0) == 43);
    assert(calls == 1);
    return 0;
}
