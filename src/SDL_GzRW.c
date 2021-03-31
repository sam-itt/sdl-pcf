#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#include "SDL_GzRW.h"

/* http://kqueue.org/blog/2012/03/16/fast-integer-overflow-detection/
 * https://github.com/ivmai/bdwgc/commit/83231d0ab5ed60015797c3d1ad9056295ac3b2bb
 * */
#define SQRT_SIZE_MAX (((size_t)(1) << (sizeof(size_t) * 8 / 2)) - 1)

#define ctx_get_zfile(ctx) ((gzFile)(ctx)->hidden.unknown.data1)

static Sint64 SDL_GzRW_size(SDL_RWops *ctx)
{
    return SDL_SetError("size() unsupported for gzip streams");
}

static Sint64 SDL_GzRW_seek(SDL_RWops *ctx, Sint64 offset, int whence)
{
    if(whence == RW_SEEK_END)
        return SDL_SetError("RW_SEEK_END unsupported for gzip streams");
    if(whence == RW_SEEK_SET) /*Would be nice to have these bound to arch's defs*/
        whence = SEEK_SET;
    if(whence == RW_SEEK_CUR)
        whence = SEEK_CUR;
    return gzseek(ctx_get_zfile(ctx), offset, whence);
}

static size_t SDL_GzRW_read(SDL_RWops *ctx, void *ptr, size_t size, size_t maxnum)
{
    if((size | maxnum) > SQRT_SIZE_MAX && size && maxnum > SIZE_MAX / size)
        return SDL_SetError("Integer overflow while trying to read %zu * %zu bytes", size, maxnum);
    return gzread(ctx_get_zfile(ctx), ptr, size * maxnum);
}

static size_t SDL_GzRW_write(SDL_RWops *ctx, const void *ptr, size_t size, size_t maxnum)
{
    if((size | maxnum) > SQRT_SIZE_MAX && size && maxnum > SIZE_MAX / size)
        return SDL_SetError("Integer overflow while trying to write %zu * %zu bytes", size, maxnum);
    return gzwrite(ctx_get_zfile(ctx), ptr, size * maxnum);
}

static int SDL_GzRW_close(SDL_RWops *ctx)
{
    return gzclose(ctx_get_zfile(ctx));
}

int SDL_GzRWEof(SDL_RWops *ctx)
{
    return(gzeof(ctx_get_zfile(ctx)));
}

SDL_RWops *SDL_RWFromGzFile(const char *filename, const char *mode)
{
    gzFile fp;
    SDL_RWops *rv;

    fp = gzopen(filename, mode);
    if(!fp){
        printf("Couldn't open %s with mode %s\n", filename, mode);
        return NULL;/*TODO: Set SDL_Error*/
    }

    rv = SDL_AllocRW();

    rv->type = SDL_RWOPS_UNKNOWN;
    rv->hidden.unknown.data1 = fp;

    rv->size = SDL_GzRW_size;
    rv->seek = SDL_GzRW_seek;
    rv->read = SDL_GzRW_read;
    rv->write = SDL_GzRW_write;
    rv->close = SDL_GzRW_close;

    return rv;
}

