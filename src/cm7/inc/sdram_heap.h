/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef _SDRAM_HEAP_H_
#define _SDRAM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SDRH_BLKSIZE  128
#define SDRH_HEAPSIZE 0x180000  // Must be agreed with linker scrpt's _SDRAM_HEAP_SIZE, and must be multiple of SDRH_BLKSIZE !!!

void* SDRH_malloc(size_t nbytes);
void SDRH_free(void* ptr);
void* SDRH_realloc(void* ptr, size_t nbytes);
void* SDRH_calloc(size_t nbytes);

#ifdef __cplusplus
}
#endif

#endif //_SDRAM_HEAP_H_
