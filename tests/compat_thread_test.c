#include "../runtime/src/compat_bridge.c"

#include <assert.h>

struct thread_observation {
    void *stack_base;
    size_t stack_size;
};

static void *observe_stack(void *argument)
{
    struct thread_observation *observation = argument;
    pthread_attr_t attributes;

    assert(pthread_getattr_np(pthread_self(), &attributes) == 0);
    assert(pthread_attr_getstack(&attributes, &observation->stack_base,
                                &observation->stack_size) == 0);
    assert(pthread_attr_destroy(&attributes) == 0);
    return argument;
}

static struct thread_observation run_guest_thread(
    const struct bionic_pthread_attr32 *guest)
{
    struct thread_observation observation = {0};
    uint32_t thread;
    void *result = NULL;

    assert(compat_pthread_create(&thread, guest, observe_stack,
                                 &observation) == 0);
    assert(compat_pthread_join(thread, &result) == 0);
    assert(result == &observation);
    return observation;
}

int main(void)
{
    struct bionic_pthread_attr32 guest;
    struct thread_observation observation;
    pthread_attr_t host;
    size_t stack_size;
    void *supplied_stack = NULL, *stack_base = NULL;
    int detached;

    assert(compat_pthread_attr_init(&guest) == 0);
    assert(compat_pthread_attr_setstacksize(&guest, 8192U) == 0);
    assert(host_attr(&guest, &host) == 0);
    assert(pthread_attr_getstacksize(&host, &stack_size) == 0);
    assert(stack_size >= 65536U && stack_size >= (size_t)PTHREAD_STACK_MIN);
    assert(guest.stack_size == 8192U);
    assert(pthread_attr_destroy(&host) == 0);
    observation = run_guest_thread(&guest);
    assert(observation.stack_size >= 65536U);

    assert(compat_pthread_attr_setstacksize(&guest, 262144U) == 0);
    assert(host_attr(&guest, &host) == 0);
    assert(pthread_attr_getstacksize(&host, &stack_size) == 0);
    assert(stack_size == 262144U);
    assert(pthread_attr_destroy(&host) == 0);
    observation = run_guest_thread(&guest);
    assert(observation.stack_size == 262144U);

    assert(posix_memalign(&supplied_stack, (size_t)sysconf(_SC_PAGESIZE),
                          262144U) == 0);
    assert(compat_pthread_attr_setstack(&guest, supplied_stack, 32768U) == 0);
    assert(host_attr(&guest, &host) == 0);
    assert(pthread_attr_getstack(&host, &stack_base, &stack_size) == 0);
    assert(stack_base == supplied_stack && stack_size == 32768U);
    assert(pthread_attr_destroy(&host) == 0);
    assert(compat_pthread_attr_setstack(&guest, supplied_stack, 262144U) == 0);
    assert(host_attr(&guest, &host) == 0);
    assert(pthread_attr_getstack(&host, &stack_base, &stack_size) == 0);
    assert(stack_base == supplied_stack && stack_size == 262144U);
    assert(pthread_attr_destroy(&host) == 0);
    observation = run_guest_thread(&guest);
    assert(observation.stack_base == supplied_stack);
    assert(observation.stack_size == 262144U);
    free(supplied_stack);

    assert(compat_pthread_attr_init(&guest) == 0);
    assert(compat_pthread_attr_setstacksize(&guest, 8192U) == 0);
    assert(compat_pthread_attr_setdetachstate(&guest, 1) == 0);
    assert(host_attr(&guest, &host) == 0);
    assert(pthread_attr_getdetachstate(&host, &detached) == 0);
    assert(detached == PTHREAD_CREATE_DETACHED);
    assert(pthread_attr_destroy(&host) == 0);
    assert(compat_pthread_attr_destroy(&guest) == 0);

    puts("PASS: Android 8 KiB thread starts; large/supplied stacks and detach state preserved");
    return 0;
}
