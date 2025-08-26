// Core/Inc/sys/times.h
#ifndef _SYS_TIMES_H
#define _SYS_TIMES_H

#ifdef __cplusplus
extern "C" {
#endif

    struct tms {
        long tms_utime;
        long tms_stime;
        long tms_cutime;
        long tms_cstime;
    };

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIMES_H */