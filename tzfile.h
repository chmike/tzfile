#include <stdint.h>
#include <stddef.h>
#include <time.h>

struct ttype_t {
    int32_t offset;
    int8_t  isdst;
    int8_t  abbridx;
};

struct leap_t {
    time_t  time;
    int32_t delta;
};

struct tzfile_t {
    char    type[20];      /* Type "TZif" */
    size_t nttime;         /* number of transition times */
    size_t nttype;         /* number of local time types */
    size_t nleap;          /* number of leap seconds */
    size_t nttisgmt;       /* number of trans. time flags */
    size_t nttisstd;       /* number of trans. time flags */
    size_t nchar;          /* number of abbr. chars */
    time_t *ttime;         /* Table of transition times */
    char   *ttidx;         /* Table of index in time offset */
    struct ttype_t *ttype; /* Table of offset and type values */
    char   *ttisstd;       /* Table of flags for trans. time std/wall */
    char   *ttisgmt;       /* Table of flags for trans. time gmt/local */
    struct leap_t *leap;   /* Table of leap seconds (8 bytes per entry) */
    char   *chars;         /* Strings */
};

struct tzfile_t *tzfile_load(const char *file_name);
