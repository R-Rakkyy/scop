/**
 * @file system_stats.h
 * @brief System stats sampling for the F3 overlay.
 * @note 24/08/2026: Linux only (uses /proc and sysconf), Vulkan only (uses VK_EXT_memory_budget).
 */

#ifndef SCOP_STATS_SYSTEM_STATS_H
#define SCOP_STATS_SYSTEM_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Process- and system-level stats for the F3 overlay, sourced from /proc
 * (Linux-only, matching this project's target platform). Any field with
 * its `*_available` flag false has no meaningful value ("n/a" in the UI). */
typedef struct {
    /* This process' CPU usage, summed across all threads. */
    float cpu_percent;
    int   cpu_available;

    /* Whole-machine CPU usage, summed across all cores */
    float sys_cpu_percent;
    int   sys_cpu_available;

    /* This process' resident set size. */
    long  rss_kb;
    int   rss_available;

    /* Whole-machine RAM: used_kb = MemTotal - MemAvailable, i.e. what's
     * actually unavailable to new allocations (matches btop/free -h's
     * "available"-based accounting, not the raw MemFree figure which
     * also counts reclaimable cache as "used"). */
    unsigned long long mem_total_kb;
    unsigned long long mem_used_kb;
    int                mem_available;

    /* Device-local (VRAM) heap usage, via VK_EXT_memory_budget if the device supports it. */
    unsigned long long gpu_mem_used_bytes;
    unsigned long long gpu_mem_budget_bytes;
    int                gpu_mem_available;
} t_SystemStats;

/* Call once per frame (or a few times a second): CPU% is a delta since
 * the previous call, so the first call always reports cpu_available=0. */
void system_stats_sample(t_SystemStats *out);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_STATS_SYSTEM_STATS_H */
