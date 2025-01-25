#pragma once

#include <stdint.h>
#include <stddef.h>

#define KB 1024
#define MB KB * 1024
#define GB MB * 1024

#define DEFAULT_ALIGN 8

typedef uint8_t Color;
enum {BLACK, RED};

typedef struct M_Block M_Block;
struct M_Block
{
    M_Block *par_next;
    M_Block *right;
    M_Block *left;
    ptrdiff_t size;
    Color col;
};

typedef struct M_Heap M_Heap;
struct M_Heap
{
    M_Block *free_list;
    M_Block *t_nil;
    
    ptrdiff_t capacity;
    ptrdiff_t committed;
    ptrdiff_t offset;
};

typedef struct TempHeap TempHeap;
struct TempHeap
{
    M_Heap *heap;
    M_Block *chain;
};

M_Heap *init_heap(ptrdiff_t reserve, ptrdiff_t commit);
void *heap_push_align(M_Heap *heap, ptrdiff_t size, ptrdiff_t align);
void *heap_zpush_align(M_Heap *heap, ptrdiff_t size, ptrdiff_t align);
#define heap_push(heap, size) heap_push_align(heap, size, DEFAULT_ALIGN)
#define heap_zpush(heap, size) heap_zpush_align(heap, size, DEFAULT_ALIGN)
void heap_free(M_Heap *heap, void *block);
void heap_clear_all(M_Heap *heap);

TempHeap begin_temp(M_Heap *heap);
void *tmp_push_align(TempHeap *tmp, ptrdiff_t size, ptrdiff_t align);
void *tmp_zpush_align(TempHeap *tmp, ptrdiff_t size, ptrdiff_t align);
#define tmp_push(tmp, size) tmp_push_align(tmp, size, DEFAULT_ALIGN)
#define tmp_zpush(tmp, size) tmp_zpush_align(tmp, size, DEFAULT_ALIGN)
void end_temp(TempHeap *tmp);
