#include "fmod_cpu.h"
#include <assert.h>

int main(void)
{
    assert(nfsmw_android_cpu_features(0) == 1);
    assert(nfsmw_android_cpu_features(1UL << 6) == 9);
    assert(nfsmw_android_cpu_features((1UL << 6) | (1UL << 13)) == 11);
    assert(nfsmw_android_cpu_features(
        (1UL << 6) | (1UL << 13) | (1UL << 12)) == 15);
    assert(nfsmw_android_cpu_features(1UL << 30) == 1);
    return 0;
}
