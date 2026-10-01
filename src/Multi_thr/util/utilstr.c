/*
 * utilstr.c: length-limited string helpers and alignment-aware memory
 * copy / fill / swap.
 */

#include "sh2.h"

/** Copies at most `limit` bytes including the terminator; returns the length copied (without it). */
int UtilStrCpyL(char *dst, char *src, int limit) {
    int len;

    for (len = 0; limit > 0; len++) {
        limit--;
        if ((*dst++ = *src++) == '\0') {
            break;
        }
    }
    return len;
}

/** Writes `s1` then `s2` into `dst`, at most `limit` bytes. Returns the length. */
int UtilStrCatL(char *dst, char *s1, char *s2, int limit) {
    int len;

    len = UtilStrCpyL(dst, s1, limit);
    len += UtilStrCpyL(dst + len, s2, limit - len);
    return len;
}

/** Writes `s1`, `s2` then `s3` into `dst`, at most `limit` bytes. Returns the length. */
int UtilStrCat3L(char *dst, char *s1, char *s2, char *s3, int limit) {
    int len;

    len = UtilStrCpyL(dst, s1, limit);
    len += UtilStrCpyL(dst + len, s2, limit - len);
    len += UtilStrCpyL(dst + len, s3, limit - len);
    return len;
}

/** dst = path1 + "/" + path2, unless path2 is absolute. */
int UtilStrPathCatL(char *dst, char *path1, char *path2, int limit) {
    int len;

    len = 0;
    if (path2 == NULL) {
        return 0;
    }
    if (*path2 != '/' && *path2 != '\\' && path1 != NULL) {
        len = UtilStrCpyL(dst, path1, limit);
        if (dst[len - 1] != '/' && *path2 != '\\') {
            len += UtilStrCpyL(dst + len, "/", limit - len);
        }
    }
    len += UtilStrCpyL(dst + len, path2, limit - len);
    return len;
}

/** Upper-cases the path and turns '/' into '\'; returns strlen - 1. */
int UtilStrConvertCdPath(char *path) {
    int len = 0;
    char ch;

    while ((ch = *path) != '\0') {
        if (ch >= 'a' && ch <= 'z') {
            *path = ch - ('a' - 'A');
        } else if (ch == '/') {
            *path = '\\';
        }
        path++;
        len++;
    }
    return len - 1;
}

/** Turns '\' into '/'; returns strlen - 1. */
int UtilStrConvertHdPath(char *path) {
    int len = 0;
    char ch;

    while ((ch = *path) != '\0') {
        if (ch == '\\') {
            *path = '/';
        }
        path++;
        len++;
    }
    return len - 1;
}

/** Copies n bytes with the widest access the alignment of dst, src and n allows. */
int UtilMemCpy(char *dst, char *src, int n) {
    int loop;
    int bit;

    bit = ((int)dst | (int)src | n) | 0x10;
    if ((bit & 0xF) == 0) {
        u_long128 *d = (u_long128 *)dst;
        u_long128 *s = (u_long128 *)src;

        loop = n / sizeof(u_long128);
        while (loop-- > 0) {
            *d++ = *s++;
        }
        return n;
    } else if ((bit & 0x7) == 0) {
        long *d = (long *)dst;
        long *s = (long *)src;

        loop = n / sizeof(long);
        while (loop-- > 0) {
            *d++ = *s++;
        }
        return n;
    } else if ((bit & 0x3) == 0) {
        int *d = (int *)dst;
        int *s = (int *)src;

        loop = n / sizeof(int);
        while (loop-- > 0) {
            *d++ = *s++;
        }
        return n;
    } else if ((bit & 0x1) == 0) {
        short *d = (short *)dst;
        short *s = (short *)src;

        loop = n / sizeof(short);
        while (loop-- > 0) {
            *d++ = *s++;
        }
        return n;
    } else {
        char *d = dst;
        char *s = src;

        loop = n;
        while (loop-- > 0) {
            *d++ = *s++;
        }
        return n;
    }
}

/** Fills n bytes with ch, with the widest access the alignment of dst and n allows. */
int UtilMemSet(char *dst, char ch, int n) {
    int loop;
    int bit;

    bit = ((int)dst | n) | 0x10;
    if ((bit & 0xF) == 0) {
        u_long128 *d = (u_long128 *)dst;
        u_long128 s[1];
        u_long128 set;

        ((unsigned long *)s)[1] = ((unsigned long *)s)[0] = (unsigned char)ch * 0x0101010101010101;
        set = s[0];
        loop = n / sizeof(u_long128);
        while (loop-- > 0) {
            *d++ = set;
        }
        return n;
    } else if ((bit & 0x7) == 0) {
        unsigned long *d = (unsigned long *)dst;
        unsigned long set;

        set = (unsigned char)ch * 0x0101010101010101;
        loop = n / sizeof(unsigned long);
        while (loop-- > 0) {
            *d++ = set;
        }
        return n;
    } else if ((bit & 0x3) == 0) {
        unsigned int *d = (unsigned int *)dst;
        unsigned int set;

        set = (unsigned char)ch * 0x01010101;
        loop = n / sizeof(unsigned int);
        while (loop-- > 0) {
            *d++ = set;
        }
        return n;
    } else if ((bit & 0x1) == 0) {
        unsigned short *d = (unsigned short *)dst;
        unsigned short set;

        set = (unsigned char)ch * 0x0101;
        loop = n / sizeof(unsigned short);
        while (loop-- > 0) {
            *d++ = set;
        }
        return n;
    } else {
        unsigned char *d = (unsigned char *)dst;
        unsigned char set;

        set = (char)(unsigned char)ch * 0x0101;
        loop = n;
        while (loop-- > 0) {
            *d++ = set;
        }
        return n;
    }
}

/** Swaps n bytes between dst and src, with the widest access the alignment allows. */
int UtilMemSwap(char *dst, char *src, int n) {
    int loop;
    int bit;

    bit = ((int)dst | (int)src | n) | 0x10;
    if ((bit & 0xF) == 0) {
        u_long128 *d = (u_long128 *)dst;
        u_long128 *s = (u_long128 *)src;
        u_long128 tmp;

        loop = n / sizeof(u_long128);
        while (loop-- > 0) {
            tmp = *d;
            *d++ = *s;
            *s++ = tmp;
        }
        return n;
    } else if ((bit & 0x7) == 0) {
        long *d = (long *)dst;
        long *s = (long *)src;
        long tmp;

        loop = n / sizeof(long);
        while (loop-- > 0) {
            tmp = *d;
            *d++ = *s;
            *s++ = tmp;
        }
        return n;
    } else if ((bit & 0x3) == 0) {
        int *d = (int *)dst;
        int *s = (int *)src;
        int tmp;

        loop = n / sizeof(int);
        while (loop-- > 0) {
            tmp = *d;
            *d++ = *s;
            *s++ = tmp;
        }
        return n;
    } else if ((bit & 0x1) == 0) {
        short *d = (short *)dst;
        short *s = (short *)src;
        short tmp;

        loop = n / sizeof(short);
        while (loop-- > 0) {
            tmp = *d;
            *d++ = *s;
            *s++ = tmp;
        }
        return n;
    } else {
        char *d = dst;
        char *s = src;
        char tmp;

        loop = n;
        while (loop-- > 0) {
            tmp = *d;
            *d++ = *s;
            *s++ = tmp;
        }
        return n;
    }
}
