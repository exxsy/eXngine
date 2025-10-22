#pragma once

#define EX_ALLOC(sz)                 malloc(sz)
#define EX_REALLOC(ptr, sz)          realloc(ptr, sz)
#define EX_FREE(ptr)                 free(ptr)
