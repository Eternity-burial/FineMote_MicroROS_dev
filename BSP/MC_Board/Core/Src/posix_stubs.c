/**
 ******************************************************************************
 * @file    posix_stubs.c
 * @author  Your Name
 * @brief   Provides POSIX function stubs to bridge different C library ABIs
 *          (ARM ABI vs Newlib ABI for libmicroros.a) in a FreeRTOS environment.
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================
//          MANUALLY PROVIDE MISSING POSIX TYPE DEFINITIONS
// =================================================================================
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

#ifndef _TIME_H_
#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
    time_t   tv_sec;
    long     tv_nsec;
};
#endif
typedef int clockid_t;
#endif

#ifndef _UNISTD_H_
typedef unsigned int useconds_t;
#endif

#ifndef EFAULT
#define EFAULT 14
#endif
#ifndef ENOSYS
#define ENOSYS 38
#endif

// =================================================================================
//      STUBS FOR NEWLIB ABI (required by libmicroros.a)
// =================================================================================
// These stubs satisfy symbols like `__errno` and `_impure_ptr` which are
// expected by libraries compiled with GCC/Newlib.

// The ARM C library provides its own `__aeabi_errno_addr`. We need to provide
// the Newlib equivalent `__errno` and make them compatible.

// 1. Define the re-entrancy structure that Newlib expects.
struct _reent {
    int _errno;
};

// 2. Create a single, static instance of this structure.
static struct _reent reent_instance;

// 3. Define `_impure_ptr`, which Newlib uses to find the re-entrancy struct.
//    It must point to our static instance.
struct _reent * const _impure_ptr = &reent_instance;

// 4. Define `__errno`, which is the Newlib way of getting the errno address.
//    We return the address of the _errno field inside our static reent struct.
//    NOTE: The name is `__errno`, NOT `__aeabi_errno_addr`.
int *__errno(void) {
    return &_impure_ptr->_errno;
}


// =================================================================================
//                    STUB FUNCTION IMPLEMENTATIONS (FreeRTOS BASED)
// =================================================================================

int usleep(useconds_t useconds)
{
    if (useconds > 0)
    {
        const uint32_t ms = useconds / 1000;
        TickType_t delay_ticks = pdMS_TO_TICKS(ms);
        if (delay_ticks == 0 && useconds > 0) {
            delay_ticks = 1;
        }
        vTaskDelay(delay_ticks);
    }
    return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    (void)clk_id;
    if (tp == NULL) {
        errno = EFAULT;
        return -1;
    }
    TickType_t current_ticks = xTaskGetTickCount();
    uint32_t msec = current_ticks * portTICK_PERIOD_MS;
    tp->tv_sec = msec / 1000;
    tp->tv_nsec = (msec % 1000) * 1000000;
    return 0;
}


// =================================================================================
//          ADDITIONAL STUBS TO SATISFY OTHER LINKER ERRORS
// =================================================================================

time_t time(time_t *t)
{
    struct timespec ts;
    clock_gettime(0, &ts);
    if (t != NULL) {
        *t = ts.tv_sec;
    }
    return ts.tv_sec;
}

void exit(int status)
{
    (void)status;
    __asm volatile("cpsid i");
    while(1) {}
}

void _atexit_init(void) {}
void _atexit_mutex(void) {}

int setenv(const char *name, const char *value, int overwrite)
{
    (void)name; (void)value; (void)overwrite;
    errno = ENOSYS;
    return -1;
}

int unsetenv(const char *name)
{
    (void)name;
    errno = ENOSYS;
    return -1;
}

#ifndef _ctype_
    char *__attribute__((weak)) _ctype_ = (char*)0;
#endif

ssize_t _write(int fd, const void *buf, size_t nbyte) {
    (void)fd; (void)buf;
    return (ssize_t)nbyte;
}

ssize_t _read(int fd, void *buf, size_t nbyte) {
    (void)fd; (void)buf; (void)nbyte;
    return 0;
}

#ifdef __cplusplus
}
#endif