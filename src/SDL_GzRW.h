#ifndef SDL_GZRW_H
#define SDL_GZRW_H
#include "SDL_rwops.h"

SDL_RWops *SDL_RWFromGzFile(const char *filename, const char *mode);
int SDL_GzRWEof(SDL_RWops *ctx);
#endif /* SDL_GZRW_H */
