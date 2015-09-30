#include "tzfile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>

uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

int16_t swap_int16(int16_t val)
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

int32_t swap_int32(int32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

int64_t swap_int64(int64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) &
            0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) &
            0x0000FFFF0000FFFFULL);
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

uint64_t swap_uint64(uint64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) &
            0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) &
            0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

struct tzfile_hdr_t {
    char type[20];        /* Type "TZif", "TZif2" or "TZif3" as of 2013 */
    int32_t ttisgmtcnt;   /* coded number of trans. time flags */
    int32_t ttisstdcnt;   /* coded number of trans. time flags */
    int32_t leapcnt;      /* coded number of leap seconds */
    int32_t timecnt;      /* coded number of transition times */
    int32_t typecnt;      /* coded number of local time types */
    int32_t charcnt;      /* coded number of abbr. chars */
};


static int32_t read_int32(char *p) {
    int32_t v = *(unsigned char *)p++;
    v = (v << 8) | *(unsigned char *)p++;
    v = (v << 8) | *(unsigned char *)p++;
    v = (v << 8) | *(unsigned char *)p++;
    return v;
}

struct tzfile_t *tzfile_load(const char *file_name) {
    FILE *f = NULL;
    char hdr[44];
    if (!(f = fopen(file_name, "rb")) ||
            fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) ||
            strcmp(hdr, "TZif2"))
        goto fail;
    size_t nttisgmt = read_int32(hdr + 20);
    size_t nttisstd = read_int32(hdr + 24);
    size_t nleap = read_int32(hdr + 28);
    size_t nttime = read_int32(hdr + 32);
    size_t nttype = read_int32(hdr + 36);
    size_t nchar = read_int32(hdr + 40);

    size_t data_size =  sizeof(time_t) * nttime + nttime +
                        sizeof(struct ttype_t) * nttype + nchar +
                        sizeof(struct leap_t) * nleap + nttisstd + nttisgmt;
    size_t struct_size = sizeof(struct tzfile_t) + data_size;

    struct tzfile_t *tzf = malloc(struct_size);
    memcpy(tzf->type, hdr, sizeof(tzf->type));
    tzf->nttime = nttime;
    tzf->nttype = nttype;
    tzf->nttisgmt = nttisgmt;
    tzf->nttisstd = nttisstd;
    tzf->nleap = nleap;
    tzf->nchar = nchar;

    /* initialize pointers */
    char *p = (char*)tzf + sizeof(struct tzfile_t), *end;
    tzf->ttime = (time_t*)p;
    tzf->ttidx = p += sizeof(time_t) * nttime;
    tzf->ttype = (struct ttype_t*)(p += nttime);
    tzf->chars = p += sizeof(struct ttype_t*) * nttype;
    tzf->leap = (struct leap_t*)(p += nchar);
    tzf->ttisstd = p += sizeof(struct leap_t*) * nleap;
    tzf->ttisgmt = p += nttisstd;

    char buf[600]; /* multiple of 4, 6 and 8 */
    size_t len;

    /* read and convert transition times */
    len = nttime * 4;
    time_t *ip = tzf->ttime;
    while (len) {
        size_t l = len < sizeof(buf) ? len : sizeof(buf);
        if (fread(buf, 1, l, f) != l)
            goto fail;
        for (p = buf, end = buf + l; p != end; p += 4, ++ip)
            *ip = read_int32(p);
        len -= l;
    }

    /* read index from transition times to transition types */
    if (fread(tzf->ttidx, 1, nttime, f) != nttime)
        goto fail;

    /* read and convert transition types */
    len = nttype * 6;
    struct ttype_t *tp = tzf->ttype;
    while (len) {
        size_t l = len < sizeof(buf) ? len : sizeof(buf);
        if (fread(buf, 1, l, f) != l)
            goto fail;
        for (p = buf, end = buf + l; p != end; p += 6, ++tp) {
            tp->offset = read_int32(p);
            tp->isdst = p[4];
            tp->abbridx = p[5];
        }
        len -= l;
    }

    /* read char strings */
    if (fread(tzf->chars, 1, nchar, f) != nchar)
        goto fail;

    /* read and convert leap seconds */
    len = nleap * 8;
    struct leap_t *lp = tzf->leap;
    while (len) {
        size_t l = len < sizeof(buf) ? len : sizeof(buf);
        if (fread(buf, 1, l, f) != l)
            goto fail;
        for (p = buf, end = buf + l; p != end; p += 8, ++lp) {
            lp->time = read_int32(p);
            lp->delta = read_int32(p + 4);
        }
        len -= l;
    }

    /* read transition time is std flags */
    if (fread(tzf->ttisstd, 1, nttisstd, f) != nttisstd)
        goto fail;

    /* read transition tame is gmt flags */
    if (fread(tzf->ttisgmt, 1, nttisgmt, f) != nttisgmt)
        goto fail;

    fclose(f);
    return tzf;
fail:
    fclose(f);
    free(tzf);
    return NULL;
}
