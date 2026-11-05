// SPDX-License-Identifier: GPL-2.0-only
/* Short, non-destructive four-CPU scheduling and shared-memory smoke test. */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WORKERS 4
#define PAYLOAD_WORDS 16
#define WALL_LIMIT_SECONDS 5

struct worker {
    int target, observed, status, error;
    uint32_t loops, samples, checksum;
    pthread_t thread;
};
static struct worker workers[WORKERS];
static atomic_uint ready, abort_run, total;
static atomic_int start_gate;
static pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t sequence, payload[PAYLOAD_WORDS];
static uint64_t stop_ns;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
        perror("clock_gettime");
        _exit(2);
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void deadline_expired(int sig)
{
    static const char message[] = "RESULT FAIL: five-second wall deadline expired\n";
    (void)sig;
    ssize_t written = write(STDERR_FILENO, message, sizeof(message) - 1);
    (void)written;
    _exit(124);
}

static uint32_t mix(uint32_t x)
{
    x ^= x >> 16;
    x *= UINT32_C(0x7feb352d);
    x ^= x >> 15;
    x *= UINT32_C(0x846ca68b);
    return x ^ (x >> 16);
}

static void fail_worker(struct worker *w, int status, int error)
{
    w->status = status;
    w->error = error;
    atomic_store_explicit(&abort_run, 1, memory_order_release);
}

static void *worker_main(void *argument)
{
    struct worker *w = argument;
    cpu_set_t mask;
    int rc;
    uint32_t value = (uint32_t)w->target + UINT32_C(0x15300000);
    CPU_ZERO(&mask);
    CPU_SET(w->target, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask)) {
        fail_worker(w, 1, errno);
        return NULL;
    }
    w->observed = sched_getcpu();
    w->samples = 1;
    if (w->observed != w->target) {
        fail_worker(w, 2, w->observed < 0 ? errno : 0);
        return NULL;
    }
    atomic_fetch_add_explicit(&ready, 1, memory_order_release);
    while (!atomic_load_explicit(&start_gate, memory_order_acquire)) {
        if (atomic_load_explicit(&abort_run, memory_order_acquire))
            return NULL;
        sched_yield();
    }
    if (atomic_load_explicit(&start_gate, memory_order_acquire) < 0)
        return NULL;
    for (;;) {
        if ((w->loops & 255U) == 0) {
            w->observed = sched_getcpu();
            ++w->samples;
            if (w->observed != w->target) {
                fail_worker(w, 2, w->observed < 0 ? errno : 0);
                break;
            }
            if (now_ns() >= stop_ns || atomic_load_explicit(&abort_run, memory_order_acquire))
                break;
        }
        for (unsigned int i = 0; i < 16; ++i)
            value = mix(value + i + w->loops);
        rc = pthread_mutex_lock(&data_lock);
        if (rc) {
            fail_worker(w, 3, rc);
            break;
        }
        bool valid = true;
        for (unsigned int i = 0; i < PAYLOAD_WORDS; ++i)
            if (payload[i] != mix(sequence ^ (UINT32_C(0x1534a700) + i)))
                valid = false;
        if (!valid) {
            (void)pthread_mutex_unlock(&data_lock);
            fail_worker(w, 4, 0);
            break;
        }
        ++sequence;
        for (unsigned int i = 0; i < PAYLOAD_WORDS; ++i)
            payload[i] = mix(sequence ^ (UINT32_C(0x1534a700) + i));
        rc = pthread_mutex_unlock(&data_lock);
        if (rc) {
            fail_worker(w, 3, rc);
            break;
        }
        atomic_fetch_add_explicit(&total, 1, memory_order_relaxed);
        ++w->loops;
    }
    w->checksum = value;
    return NULL;
}

static bool parse_number(const char *text, unsigned long max, unsigned long *out)
{
    char *end;
    errno = 0;
    if (!*text || *text < '0' || *text > '9')
        return false;
    *out = strtoul(text, &end, 10);
    return !errno && !*end && *out <= max;
}

static bool parse_targets(const char *argument)
{
    char buffer[128], *part, *save = NULL;
    unsigned int count = 0;
    if (strlen(argument) >= sizeof(buffer))
        return false;
    strcpy(buffer, argument);
    for (part = strtok_r(buffer, ",", &save); part; part = strtok_r(NULL, ",", &save)) {
        unsigned long id;
        if (count == WORKERS || !parse_number(part, CPU_SETSIZE - 1, &id))
            return false;
        for (unsigned int i = 0; i < count; ++i)
            if (workers[i].target == (int)id)
                return false;
        workers[count++].target = (int)id;
    }
    /* Reject empty comma fields, which strtok would otherwise skip. */
    return count == WORKERS && argument[0] != ',' &&
           argument[strlen(argument) - 1] != ',' && !strstr(argument, ",,");
}

static bool parse_online(const char *text, cpu_set_t *mask)
{
    const char *p = text;
    CPU_ZERO(mask);
    while (*p) {
        char *end;
        unsigned long first, last;
        if (*p < '0' || *p > '9')
            return false;
        errno = 0;
        first = strtoul(p, &end, 10);
        if (errno || first >= CPU_SETSIZE)
            return false;
        p = end;
        last = first;
        if (*p == '-') {
            p++;
            if (*p < '0' || *p > '9')
                return false;
            last = strtoul(p, &end, 10);
            if (errno || last < first || last >= CPU_SETSIZE)
                return false;
            p = end;
        }
        for (unsigned long i = first; i <= last; ++i)
            CPU_SET((int)i, mask);
        if (!*p)
            return true;
        if (*p++ != ',' || !*p)
            return false;
    }
    return false;
}

int main(int argc, char **argv)
{
    unsigned long duration_ms = 2000;
    unsigned int created = 0;
    bool explicit_cpus = false, bad = false;
    uint64_t begin_ns, sum = 0;
    cpu_set_t online, allowed;
    char online_text[4096];
    FILE *file;
    struct sigaction sa = { .sa_handler = deadline_expired };
    for (unsigned int i = 0; i < WORKERS; ++i) {
        workers[i].target = (int)i;
        workers[i].observed = -1;
    }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--duration-ms") && i + 1 < argc) {
            if (!parse_number(argv[++i], 3000, &duration_ms) || duration_ms < 100)
                goto usage;
        } else if (!strcmp(argv[i], "--cpus") && i + 1 < argc) {
            explicit_cpus = true;
            if (!parse_targets(argv[++i]))
                goto usage;
        } else {
            goto usage;
        }
    }
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL)) {
        perror("sigaction");
        return 2;
    }
    alarm(WALL_LIMIT_SECONDS);
    begin_ns = now_ns();
    file = fopen("/sys/devices/system/cpu/online", "r");
    if (!file || !fgets(online_text, sizeof(online_text), file)) {
        perror("read online CPUs");
        return 2;
    }
    fclose(file);
    online_text[strcspn(online_text, "\r\n")] = '\0';
    if (!parse_online(online_text, &online) || sched_getaffinity(0, sizeof(allowed), &allowed)) {
        fprintf(stderr, "RESULT FAIL: cannot determine online/permitted CPUs\n");
        return 2;
    }
    printf("MODE=%s ONLINE=%s TARGET=%d,%d,%d,%d DURATION_MS=%lu\n",
           explicit_cpus ? "explicit-cpus" : "t153-cpu0-3", online_text,
           workers[0].target, workers[1].target, workers[2].target, workers[3].target, duration_ms);
    fflush(stdout);
    for (unsigned int i = 0; i < WORKERS; ++i) {
        int id = workers[i].target;
        if (!CPU_ISSET(id, &online) || !CPU_ISSET(id, &allowed)) {
            fprintf(stderr, "RESULT FAIL: target CPU %d is offline or outside permitted affinity\n", id);
            return 1;
        }
    }
    for (unsigned int i = 0; i < PAYLOAD_WORDS; ++i)
        payload[i] = mix(UINT32_C(0x1534a700) + i);
    for (unsigned int i = 0; i < WORKERS; ++i) {
        int rc = pthread_create(&workers[i].thread, NULL, worker_main, &workers[i]);
        if (rc) {
            fail_worker(&workers[i], 5, rc);
            bad = true;
            break;
        }
        ++created;
    }
    uint64_t setup_deadline = now_ns() + UINT64_C(500000000);
    while (atomic_load_explicit(&ready, memory_order_acquire) != created &&
           !atomic_load_explicit(&abort_run, memory_order_acquire) && now_ns() < setup_deadline)
        usleep(1000);
    if (created != WORKERS || atomic_load_explicit(&ready, memory_order_acquire) != WORKERS ||
        atomic_load_explicit(&abort_run, memory_order_acquire)) {
        bad = true;
        atomic_store_explicit(&abort_run, 1, memory_order_release);
        atomic_store_explicit(&start_gate, -1, memory_order_release);
    } else {
        stop_ns = now_ns() + duration_ms * UINT64_C(1000000);
        atomic_store_explicit(&start_gate, 1, memory_order_release);
    }
    for (unsigned int i = 0; i < created; ++i) {
        int rc = pthread_join(workers[i].thread, NULL);
        if (rc) {
            fprintf(stderr, "RESULT FAIL: pthread_join CPU %d error=%d\n", workers[i].target, rc);
            return 1;
        }
    }
    for (unsigned int i = 0; i < WORKERS; ++i) {
        struct worker *w = &workers[i];
        bool pass = i < created && w->status == 0 && w->loops > 0 && w->samples > 1 && w->observed == w->target;
        printf("CPU target=%d observed=%d samples=%" PRIu32 " loops=%" PRIu32 " checksum=%08" PRIx32 " status=%s error_kind=%d errno=%d\n",
               w->target, w->observed, w->samples, w->loops, w->checksum, pass ? "PASS" : "FAIL", w->status, w->error);
        sum += w->loops;
        bad |= !pass;
    }
    unsigned int atomic_count = atomic_load_explicit(&total, memory_order_relaxed);
    bad |= atomic_load_explicit(&abort_run, memory_order_acquire) != 0 || sum > UINT32_MAX ||
           sequence != sum || atomic_count != sum;
    for (unsigned int i = 0; i < PAYLOAD_WORDS; ++i)
        bad |= payload[i] != mix(sequence ^ (UINT32_C(0x1534a700) + i));
    printf("SHARED sequence=%" PRIu32 " atomic_count=%u expected=%" PRIu64 "\n", sequence, atomic_count, sum);
    printf("RESULT %s wall_ms=%" PRIu64 "\n", bad ? "FAIL" : "PASS", (now_ns() - begin_ns) / UINT64_C(1000000));
    alarm(0);
    return bad ? 1 : 0;
usage:
    fprintf(stderr, "Usage: %s [--duration-ms 100..3000] [--cpus A,B,C,D]\n", argv[0]);
    return 2;
}
