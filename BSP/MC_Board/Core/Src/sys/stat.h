#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/*
 * A minimal sys/stat.h definition to allow syscalls.c to compile
 * when the toolchain does not provide a full POSIX-compliant version.
 * The content of this struct is not important for our stub functions,
 * it just needs to exist.
 */

#ifdef __cplusplus
extern "C" {
#endif

    typedef unsigned short mode_t;

    struct stat {
        mode_t st_mode;
        /* Add other dummy members if the compiler complains about size */
        int    st_dev;
        int    st_ino;
        int    st_nlink;
        int    st_uid;
        int    st_gid;
        int    st_rdev;
        long   st_size;
    };

    /* Define the minimal macros needed by the syscalls.c stubs */
#define S_IFCHR 0020000 /* character special */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STAT_H */