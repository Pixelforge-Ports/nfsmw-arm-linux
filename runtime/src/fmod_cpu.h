#ifndef NFSMW_FMOD_CPU_H
#define NFSMW_FMOD_CPU_H

#include <stdint.h>

/* Linux ARM32 HWCAP -> the bundled Android NDK cpu-features bit layout. */
static inline uint64_t nfsmw_android_cpu_features(unsigned long hwcap)
{
    uint64_t features = 1U; /* This runtime requires ARMv7. */
    if (hwcap & (1UL << 6)) features |= 1U << 3; /* VFPv2 */
    if (hwcap & (1UL << 13)) features |= 1U << 1; /* VFPv3 */
    if (hwcap & (1UL << 12)) features |= 1U << 2; /* NEON */
    return features;
}

#endif
