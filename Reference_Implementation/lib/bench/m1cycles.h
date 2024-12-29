#ifndef M1CYCLES_H
#define M1CYCLES_H

// For Apple M1: https://gist.github.com/dougallj/5bafb113492047c865c0c8cfbc930155#file-m1_robsize-c-L390
#include <dlfcn.h>
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define KPERF_LIST                                                                                                     \
    /*  ret, name, params */                                                                                           \
    F(int, kpc_get_counting, void)                                                                                     \
    F(int, kpc_force_all_ctrs_set, int)                                                                                \
    F(int, kpc_set_counting, uint32_t)                                                                                 \
    F(int, kpc_set_thread_counting, uint32_t)                                                                          \
    F(int, kpc_set_config, uint32_t, void *)                                                                           \
    F(int, kpc_get_config, uint32_t, void *)                                                                           \
    F(int, kpc_set_period, uint32_t, void *)                                                                           \
    F(int, kpc_get_period, uint32_t, void *)                                                                           \
    F(uint32_t, kpc_get_counter_count, uint32_t)                                                                       \
    F(uint32_t, kpc_get_config_count, uint32_t)                                                                        \
    F(int, kperf_sample_get, int *)                                                                                    \
    F(int, kpc_get_thread_counters, int, unsigned int, void *)

#define F(ret, name, ...)                                                                                              \
    typedef ret name##proc(__VA_ARGS__);                                                                               \
    static name##proc *name;
KPERF_LIST
#undef F

#define CFGWORD_EL0A32EN_MASK (0x10000)
#define CFGWORD_EL0A64EN_MASK (0x20000)
#define CFGWORD_EL1EN_MASK (0x40000)
#define CFGWORD_EL3EN_MASK (0x80000)
#define CFGWORD_ALLMODES_MASK (0xf0000)

#define CPMU_NONE 0
#define CPMU_CORE_CYCLE 0x02
#define CPMU_INST_A64 0x8c
#define CPMU_INST_BRANCH 0x8d
#define CPMU_SYNC_DC_LOAD_MISS 0xbf
#define CPMU_SYNC_DC_STORE_MISS 0xc0
#define CPMU_SYNC_DTLB_MISS 0xc1
#define CPMU_SYNC_ST_HIT_YNGR_LD 0xc4
#define CPMU_SYNC_BR_ANY_MISP 0xcb
#define CPMU_FED_IC_MISS_DEM 0xd3
#define CPMU_FED_ITLB_MISS 0xd4

#define KPC_CLASS_FIXED (0)
#define KPC_CLASS_CONFIGURABLE (1)
#define KPC_CLASS_POWER (2)
#define KPC_CLASS_RAWPMU (3)
#define KPC_CLASS_FIXED_MASK (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_CLASS_POWER_MASK (1u << KPC_CLASS_POWER)
#define KPC_CLASS_RAWPMU_MASK (1u << KPC_CLASS_RAWPMU)

#define COUNTERS_COUNT 10
#define CONFIG_COUNT 8
#define KPC_MASK (KPC_CLASS_CONFIGURABLE_MASK | KPC_CLASS_FIXED_MASK)
uint64_t g_counters[COUNTERS_COUNT];
uint64_t g_config[COUNTERS_COUNT];


static void kperf_init(void) {
    void *kperf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/Versions/A/kperf", RTLD_LAZY);
    if (!kperf) {
        printf("[ERROR] kperf_init: dlopen() failed!\n");
        return;
    }
#define F(ret, name, ...)                                                                                              \
    name = (name##proc *) (dlsym(kperf, #name));                                                                       \
    if (!name) {                                                                                                       \
        printf("[ERROR] kperf_init: failed to load symbol: %s\n", #name);                                              \
        return;                                                                                                        \
    }
    KPERF_LIST
#undef F

    // TODO: KPC_CLASS_RAWPMU_MASK
    const unsigned c1 = kpc_get_counter_count(KPC_MASK);
    if (c1 != CONFIG_COUNT) {
        printf("[ERROR] kperf_init: wrong fixed counters count: %u\n", c1);
        return;
    }

    const unsigned c2 = kpc_get_config_count(KPC_MASK);
    if (c2 != 6) {
        printf("[ERROR] kperf_init: wrong fixed config count: %u\n", c2);
        return;
    }

    if (kpc_force_all_ctrs_set(1)) {
        printf("[ERROR] kperf_init: kpc_force_all_ctrs_set failed (root?)\n");
        return;
    }

    g_config[0] = CPMU_CORE_CYCLE | CFGWORD_EL0A64EN_MASK;

    if (kpc_set_config(KPC_MASK, g_config)) {
        printf("[ERROR] kperf_init: kpc_set_config failed (root?)\n");
        return;
    }

    if (kpc_set_counting(KPC_MASK)) {
        printf("[ERROR] kperf_init: kpc_set_counting failed (root?)\n");
        return;
    }

    if (kpc_set_thread_counting(KPC_MASK)) {
        printf("[ERROR] kperf_init: kpc_set_thread_counting failed (root?)\n");
        return;
    }
}

static void kperf_init_once(void) {
    static pthread_once_t init_static_once = PTHREAD_ONCE_INIT;
    pthread_once(&init_static_once, kperf_init);
}


static unsigned long long int __m1_rdtsc(void) {
    if (kpc_get_thread_counters(0, COUNTERS_COUNT, g_counters)) {
        printf("kpc_get_thread_counters failed\n");
        return 0;
    }
    return g_counters[2];
}

//  End

#endif