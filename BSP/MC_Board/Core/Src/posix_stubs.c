/**
 * @file posix_stubs.c
 * @brief Stub implementations for POSIX functions not available on FreeRTOS.
 *
 * This file provides minimal replacements for setenv/unsetenv
 * and defines a _ctype_ table to satisfy linking against rcutils/rmw.
 *
 * ⚠️ NOTE: These are stubs! They do not provide full functionality.
 * They only exist to let micro-ROS compile & link successfully on bare-metal.
 */

#include <stddef.h>
#include <string.h>

/* ========================================================================= */
/* Minimal stubs for setenv / unsetenv                                       */
/* ========================================================================= */

int setenv(const char *name, const char *value, int overwrite)
{
    (void)name;
    (void)value;
    (void)overwrite;
    /* On MCU, environment variables do not exist.
       Pretend success. */
    return 0;
}

int unsetenv(const char *name)
{
    (void)name;
    /* On MCU, nothing to unset. Pretend success. */
    return 0;
}

/* ========================================================================= */
/* Provide a dummy _ctype_ table (used by isalpha, isdigit, etc.)            */
/* ========================================================================= */

/*
 * Some ROS2 validation functions indirectly rely on `_ctype_`.
 * On Linux/glibc this is provided by libc, but ARMCLANG / bare-metal has no such symbol.
 * We define a simple table that classifies ASCII characters.
 */

unsigned char _ctype_[256] = { 0 };

static void init_ctype(void) __attribute__((constructor));
static void init_ctype(void)
{
    for (int c = '0'; c <= '9'; ++c) {
        _ctype_[(unsigned char)c] |= 0x04; // digit
    }
    for (int c = 'A'; c <= 'Z'; ++c) {
        _ctype_[(unsigned char)c] |= 0x01; // upper
    }
    for (int c = 'a'; c <= 'z'; ++c) {
        _ctype_[(unsigned char)c] |= 0x02; // lower
    }
    _ctype_[' ']  |= 0x08; // space
    _ctype_['\t'] |= 0x08; // space
    _ctype_['\n'] |= 0x08; // space
}

/* ========================================================================= */
/* End of file                                                               */
/* ========================================================================= */
