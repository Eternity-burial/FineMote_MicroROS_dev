/**
 ******************************************************************************
 * @file    posix_stubs.c
 * @author  Your Name
 * @brief   Provides POSIX function stubs for linking pre-compiled libraries
 *          (like libmicroros.a) in a FreeRTOS environment. These stubs map
 *          POSIX calls to their equivalent FreeRTOS API calls.
 ******************************************************************************
 */

#include "FreeRTOS.h"   // Must be included first
#include "task.h"       // For vTaskDelay, xTaskGetTickCount

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

// Ensure this file is compatible with C++ linkage if included from a C++ file
#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================
//          MANUALLY PROVIDE MISSING POSIX TYPE DEFINITIONS
// =================================================================================
// (These are still needed as FreeRTOS does not define them)

#ifndef _SYS_TYPES_H_
typedef long int __time_t;
typedef long int __suseconds_t;
#endif

#ifndef _TIME_H_
typedef int clockid_t;
struct timespec {
    __time_t tv_sec;
    long     tv_nsec;
};
#endif

#ifndef _UNISTD_H_
typedef __suseconds_t useconds_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int32_t _ssize_t;
#endif

#ifndef EFAULT
#define EFAULT 14
#endif

#ifndef ENOSYS
#define ENOSYS 38
#endif

// =================================================================================
//                    STUB FUNCTION IMPLEMENTATIONS (FreeRTOS BASED)
// =================================================================================

/**
 * @brief A FreeRTOS-based implementation of usleep.
 *        This will suspend the calling task, not the entire system.
 */
int usleep(useconds_t useconds)
{
    if (useconds > 0)
    {
        // Convert microseconds to FreeRTOS ticks.
        // Add (portTICK_PERIOD_MS * 1000 - 1) to round up.
        // Ensure we delay for at least 1 tick if useconds is not zero.
        TickType_t delay_ticks = (useconds + (configTICK_RATE_HZ * 1000 - 1)) / (configTICK_RATE_HZ * 1000);

        // A simpler but less accurate way for millisecond resolution:
        // TickType_t delay_ticks = (useconds / 1000) / portTICK_PERIOD_MS;
        // if (delay_ticks == 0 && useconds > 0) {
        //     delay_ticks = 1;
        // }

        vTaskDelay(delay_ticks > 0 ? delay_ticks : 1);
    }
    return 0;
}

/**
 * @brief A FreeRTOS-based implementation of clock_gettime.
 *        Uses the FreeRTOS tick count as the time source.
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    (void)clk_id;

    if (tp == NULL) {
        errno = EFAULT;
        return -1;
    }

    TickType_t current_ticks = xTaskGetTickCount();

    // Convert ticks to seconds and nanoseconds.
    // portTICK_PERIOD_MS is the duration of a tick in milliseconds.
    uint32_t msec = current_ticks * portTICK_PERIOD_MS;

    tp->tv_sec = msec / 1000;
    tp->tv_nsec = (msec % 1000) * 1000000;

    return 0;
}

/**
 * @brief Stub for setenv.
 */
int setenv(const char *name, const char *value, int overwrite)
{
    (void)name; (void)value; (void)overwrite;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Stub for unsetenv.
 */
int unsetenv(const char *name)
{
    (void)name;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Stub for _ctype_ array pointer.
 */
#ifndef _ctype_
    char *__attribute__((weak)) _ctype_ = (char*)0;
#endif

/* Minimal stubs for stdio file operations if needed by the library */
_ssize_t _write(int fd, const void *buf, size_t nbyte) {
    (void)fd; (void)buf;
    return (_ssize_t)nbyte;
}

_ssize_t _read(int fd, void *buf, size_t nbyte) {
    (void)fd; (void)buf; (void)nbyte;
    return 0;
}

#ifdef __cplusplus
}
#endif