/* anonseed.c - LD_PRELOAD shim for the staged cc1plus (GCC 2.95.2).
 *
 * g++ 2.95 names an anonymous namespace `_GLOBAL_.N.<file><6 chars>`:
 * append_random_chars (gcc/tree.c) adds (tv_usec << 16) ^ tv_sec ^ getpid()
 * to a static sum and spells the sum in six base-62 digits, which encode all
 * 32 bits. The retail names therefore prove the sum of each unit's compile
 * (config/retail/h3maped-loki/anonymous.tsv), not how it split between the
 * clock and the process id. When HOMM3_LOKI_TIMEOFDAY ("sec.usec") or
 * HOMM3_LOKI_PID is set, gettimeofday and getpid return those values; the
 * build sets the whole sum as seconds and a process id of 0. Otherwise both
 * calls pass through. cc1plus calls them nowhere else.
 *
 * Built by homm3.loki.toolchain with the staged gcc against glibc 2.1.3.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int gettimeofday(struct timeval* tv, struct timezone* tz)
{
    const char* fixed = getenv("HOMM3_LOKI_TIMEOFDAY");
    if (fixed != NULL && tv != NULL) {
        char* end;
        tv->tv_sec = (long) strtoul(fixed, &end, 10);
        tv->tv_usec = *end == '.' ? (long) strtoul(end + 1, NULL, 10) : 0;
        return 0;
    } else {
        int (*real)(struct timeval*, struct timezone*) =
            (int (*)(struct timeval*, struct timezone*)) dlsym(RTLD_NEXT, "gettimeofday");
        return real(tv, tz);
    }
}

pid_t getpid(void)
{
    const char* fixed = getenv("HOMM3_LOKI_PID");
    if (fixed != NULL) {
        return (pid_t) strtoul(fixed, NULL, 10);
    } else {
        pid_t (*real)(void) = (pid_t (*)(void)) dlsym(RTLD_NEXT, "getpid");
        return real();
    }
}
