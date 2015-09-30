#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "tzfile.h"

int main(void)
{
    const char *file_name = "localtime";
    struct tzfile_t *tzf = tzfile_load(file_name);
    if (tzf == NULL) {
        fprintf(stderr, "%s : %s\n", strerror(errno), file_name);
        return 1;
    }

    size_t i;
    printf("Type :     %s\n",  tzf->type);
    printf("nttime :   %zd\n", tzf->nttime);
    printf("nttype :   %zd\n", tzf->nttype);
    printf("nchar :    %zd\n", tzf->nchar);
    printf("nttisstd : %zd\n", tzf->nttisstd);
    printf("nttisgmt : %zd\n", tzf->nttisgmt);
    for (i = 0; i < tzf->nttime; ++i)
        printf("  %3zd : %11lld %2d\n", i, (long long)tzf->ttime[i], tzf->ttidx[i]);
    for (i = 0; i < tzf->nttype; ++i) {
        struct ttype_t *tp = tzf->ttype + i;
        printf("  %3zd : offset=%4lld isdst=%d abbr=%-4s isstd=%d isgmt=%d\n", i,
               (long long)tp->offset, tp->isdst, tzf->chars + tp->abbridx,
               tzf->ttisstd[i], tzf->ttisgmt[i]);
    }
    printf("\n");

    free(tzf);

    return 0;
}

