/**
 * @file system_stats.c
 * @brief System statistics sampling functions.
 * @note Linux only implementation.
 */

#define _POSIX_C_SOURCE 199309L // for clock_gettime
#include "utils/stats/system_stats.h"
#include "engine/vk/context.h"

#include <vulkan/vulkan.h>      // for VkPhysicalDeviceMemoryBudgetPropertiesEXT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Read /proc/self/stat to get this process' CPU usage in clock ticks. */
static int read_self_stat_ticks(unsigned long long *out_ticks)
{
    FILE *f     =   fopen("/proc/self/stat", "r");
    if (!f)         return 0;

    /* Field 14 = utime, 15 = stime (both in clock ticks).*/
    char buf[2048];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return 0; }
    fclose(f);

    char *after_comm    = strrchr(buf, ')'); // comm is in parentheses, and can contain spaces
    if (!after_comm)    return 0;
    after_comm          += 1;
    unsigned long long utime = 0, stime = 0;
    int field                = 2; /* state is field 3, the first one in after_comm */
    char *tok = strtok(after_comm, " \t\n");
    while (tok) {
        field++;
        if (field == 14) utime = strtoull(tok, NULL, 10);
        if (field == 15) { stime = strtoull(tok, NULL, 10); break; }
        tok = strtok(NULL, " \t\n");
    }

    if (field < 15) return 0;
    *out_ticks = utime + stime;
    return 1;
}

/* First line of /proc/stat: "cpu  user nice system idle iowait irq softirq
 * steal guest guest_nice" (all in clock ticks, summed across cores already).
 * Busy = everything except idle+iowait. */
static int read_sys_cpu_ticks(unsigned long long *out_busy, unsigned long long *out_total)
{
    FILE *f     =   fopen("/proc/stat", "r");
    if (!f)         return 0;

    /* Read the first line, which is the aggregate CPU line. */
    char buf[2048];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return 0; }
    fclose(f);

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    int n       = sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    if (n < 4)  return 0;
    if (n < 8)  { irq = softirq = steal = 0; }

    *out_total  = user + nice + system + idle + iowait + irq + softirq + steal;
    *out_busy   = *out_total - idle - iowait;
    return 1;
}

/* Read /proc/meminfo to get total and available memory in kB. */
static int read_sys_mem_kb(unsigned long long *out_total_kb, unsigned long long *out_used_kb)
{
    FILE *f     =   fopen("/proc/meminfo", "r");
    if (!f)         return 0;

    /* Read lines until we find MemTotal and MemAvailable. */
    char line[2048];
    unsigned long long total_kb = 0, avail_kb = 0;
    int have_total = 0, have_avail = 0;
    while (fgets(line, sizeof(line), f) && !(have_total && have_avail)) {
        if (sscanf(line, "MemTotal: %llu kB", &total_kb) == 1) { have_total = 1; continue; }
        if (sscanf(line, "MemAvailable: %llu kB", &avail_kb) == 1) { have_avail = 1; continue; }
    }
    fclose(f);
    if (!have_total || !have_avail) return 0;

    *out_total_kb   = total_kb;
    *out_used_kb    = total_kb > avail_kb ? total_kb - avail_kb : 0;
    return 1;
}
/* Read /proc/self/status to get this process' resident set size (VmRSS) in kB. */
static int read_rss_kb(long *out_kb)
{
    FILE *f     =   fopen("/proc/self/status", "r");
    if (!f)         return 0;

    /* Read lines until we find VmRSS. */
    char line[2048];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        long kb = 0;
        if (sscanf(line, "VmRSS: %ld kB", &kb) == 1) { *out_kb = kb; found = 1; break; }
    }
    fclose(f);
    return found;
}

/* Read GPU memory usage information using Vulkan API. This is NOT a good and practical approach. */
static void read_gpu_memory(t_SystemStats *out)
{
    if (!g_memory_budget_supported || g_physical == VK_NULL_HANDLE) return;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget    = {0};
    budget.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    VkPhysicalDeviceMemoryProperties2 mem2              = {0};
    mem2.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    mem2.pNext      = &budget;

    vkGetPhysicalDeviceMemoryProperties2(g_physical, &mem2);

    unsigned long long used = 0, avail = 0;
    for (uint32_t i = 0; i < mem2.memoryProperties.memoryHeapCount; ++i) {
        if (mem2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            used    += budget.heapUsage[i];
            avail   += budget.heapBudget[i];
        }
    }
    out->gpu_mem_used_bytes     = used;
    out->gpu_mem_budget_bytes   = avail;
    out->gpu_mem_available      = 1;
}

void system_stats_sample(t_SystemStats *out)
{
    static unsigned long long   last_ticks = 0;
    static struct timespec      last_time = {0, 0};
    static int                  have_last = 0;

    static unsigned long long   last_sys_busy = 0, last_sys_total = 0;
    static int                  have_last_sys = 0;

    memset(out, 0, sizeof(*out)); // reset all fields to zero, including availability flags

    /* **************** PROCESS RESIDENT SET SIZE **************** */
    long rss_kb = 0;
    if (read_rss_kb(&rss_kb)) { out->rss_kb = rss_kb; out->rss_available = 1; }
    /* ********************** SYSTEM MEMORY ********************** */
    unsigned long long mem_total_kb = 0, mem_used_kb = 0;
    if (read_sys_mem_kb(&mem_total_kb, &mem_used_kb)) {
        out->mem_total_kb = mem_total_kb;
        out->mem_used_kb = mem_used_kb;
        out->mem_available = 1;
    }
    /* ********************** CPU USAGE STATS ********************* */
    unsigned long long  ticks   = 0;
    struct timespec     now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (read_self_stat_ticks(&ticks) && have_last) {
        double          dt      = (double)(now.tv_sec - last_time.tv_sec) + (double)(now.tv_nsec - last_time.tv_nsec) / 1e9;
        double          dticks  = (double)(ticks - last_ticks);
        long            clk_tck = sysconf(_SC_CLK_TCK);

        if (dt > 0.0 && clk_tck > 0) {
            out->cpu_percent    = (float)((dticks / (double)clk_tck) / dt * 100.0);
            out->cpu_available  = 1;
        }
    }
    if (ticks != 0 || !have_last) {
        last_ticks  = ticks;
        last_time   = now;
        have_last   = 1;
    }
    /* ****************** SYSTEM-WIDE CPU USAGE ******************** */
    unsigned long long      sys_busy    = 0, sys_total = 0;
    if (read_sys_cpu_ticks(&sys_busy, &sys_total) && have_last_sys) {
        unsigned long long  dbusy       = sys_busy - last_sys_busy;
        unsigned long long  dtotal      = sys_total - last_sys_total;

        if (dtotal > 0) {
            out->sys_cpu_percent        = (float)((double)dbusy / (double)dtotal * 100.0);
            out->sys_cpu_available      = 1;
        }
    }
    if (sys_total != 0 || !have_last_sys) {
        last_sys_busy   = sys_busy;
        last_sys_total  = sys_total;
        have_last_sys   = 1;
    }
    /* ****************** GPU MEMORY USAGE ******************** */
    read_gpu_memory(out);
}
