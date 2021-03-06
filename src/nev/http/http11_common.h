#ifndef _http11_common_h
#define _http11_common_h

#include <stddef.h>  // size_t

typedef void (*element_cb)(void* data, const char* at, size_t length);
typedef void (*field_cb)(void* data,
                         const char* field,
                         size_t flen,
                         const char* value,
                         size_t vlen);

#endif  // _http11_common_h
