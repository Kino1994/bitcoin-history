// nov08-rebuild - Win32 -> POSIX compatibility layer.
// Lets us COMPILE AND VERIFY Satoshi's original Windows code (Nov 2008) natively
// on Linux. NOT used when cross-compiling for Windows XP with MinGW: there the
// code uses the real Win32 APIs. Active only when !WIN32.
#ifndef BITCOIN_COMPAT_H
#define BITCOIN_COMPAT_H

#if !defined(WIN32) && !defined(_WIN32)

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---- Basic Windows types ----
typedef int                 BOOL;
typedef unsigned long       DWORD;
typedef void*               HANDLE;
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// ---- Critical section -> recursive pthread mutex ----
typedef pthread_mutex_t CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(cs, &attr);
    pthread_mutexattr_destroy(&attr);
}
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(cs); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_lock(cs); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs)  { pthread_mutex_unlock(cs); }
inline bool TryEnterCriticalSection(CRITICAL_SECTION* cs) { return pthread_mutex_trylock(cs) == 0; }

// ---- Threads: _beginthread(fn, stack, arg) ----
inline unsigned long _beginthread(void (*fn)(void*), unsigned, void* arg) {
    pthread_t t;
    if (pthread_create(&t, NULL, (void*(*)(void*))fn, arg) != 0)
        return (unsigned long)-1;
    pthread_detach(t);
    return (unsigned long)t;
}

// ---- Time / sleep ----
inline void Sleep(unsigned int ms) { usleep((useconds_t)ms * 1000); }
inline unsigned int GetTickCount() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
typedef long long LARGE_INTEGER;
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* p) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    *p = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec; return TRUE;
}

// ---- Thread priority (no-op) ----
#define THREAD_PRIORITY_LOWEST          (-2)
#define THREAD_PRIORITY_BELOW_NORMAL    (-1)
#define THREAD_PRIORITY_NORMAL          0
inline HANDLE GetCurrentThread() { return (HANDLE)0; }
inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }

// ---- Debugging / misc ----
inline void DebugBreak() {}
inline void OutputDebugString(const char* s) { fputs(s, stderr); }
inline unsigned long GetModuleFileName(void*, char* buf, unsigned long n) {
    ssize_t r = readlink("/proc/self/exe", buf, n - 1);
    if (r < 0) { buf[0] = '\0'; return 0; }
    buf[r] = '\0'; return (unsigned long)r;
}
#define _HEAPOK 0
inline int _heapchk() { return _HEAPOK; }

// ---- Size-limited printf ----
#define _vsnprintf  vsnprintf
#ifndef _snprintf
#define _snprintf   snprintf
#endif

// ---- OpenSSL RAND_screen is Windows-only; no-op on POSIX ----
#define RAND_screen() ((void)0)

#endif // !WIN32
#endif // BITCOIN_COMPAT_H
