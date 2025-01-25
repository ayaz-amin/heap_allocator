#include "alloc.h"
#include <assert.h>
#include <windows.h>

ptrdiff_t align_pow_2(ptrdiff_t size, ptrdiff_t align)
{
    return (size + align - 1) & -align;
}

ptrdiff_t calc_padding_with_header(ptrdiff_t offset, ptrdiff_t align, ptrdiff_t header_size)
{
    assert((align & (align - 1)) == 0);
    ptrdiff_t padding = -offset & (align - 1);

    if(padding < header_size)
    {
        ptrdiff_t additional_space = header_size - padding;
        padding += align_pow_2(additional_space, align);
    }

    return padding;
}

void left_rotate(M_Heap *heap, M_Block *node)
{
    M_Block *y = node->right;
    node->right = y->left;
    if(y->left != heap->t_nil)
    {
        y->left->par_next = node;
    }

    y->par_next = node->par_next;

    if(node->par_next == heap->t_nil)
    {
        heap->free_list = y;
    }

    else if(node == node->par_next->left)
    {
        node->par_next->left = y;
    }

    else node->par_next->right = y;
    y->left = node;
    node->par_next = y;
}

void right_rotate(M_Heap *heap, M_Block *node)
{
    M_Block *x = node->left;
    node->left = x->right;
    if(x->right != heap->t_nil)
    {
        x->right->par_next = node;
    }

    x->par_next = node->par_next;

    if(node->par_next == heap->t_nil)
    {
        heap->free_list = x;
    }

    else if(node == node->par_next->left)
    {
        node->par_next->left = x;
    }

    else node->par_next->right = x;
    x->right = node;
    node->par_next = x;
}

void rb_transplant(M_Heap *heap, M_Block *u, M_Block *v)
{
    if(u->par_next == heap->t_nil)
    {
        heap->free_list = v;
    }

    else if(u == u->par_next->left)
    {
        u->par_next->left = v;
    }

    else u->par_next->right = v;
    v->par_next = u->par_next;
}

M_Block *rb_minimum(M_Heap *heap, M_Block *node)
{
    for(; node->left != heap->t_nil;)
    {
        node = node->left;
    }

    return node;
}

void pop_fixup(M_Heap *heap, M_Block *node)
{
    M_Block *w = 0;
    for(; node != heap->free_list && node->col == BLACK;)
    {
        if(node == node->par_next->left)
        {
            w = node->par_next->right;
            if(w->col == RED)
            {
                w->col = BLACK;
                node->par_next->col = RED;
                left_rotate(heap, node->par_next);
                w = node->par_next->right;
            }

            if(w->left->col == BLACK && w->right->col == BLACK)
            {
                w->col = RED;
                node = node->par_next;
            }

            else
            {
                if(w->right->col == BLACK)
                {
                    w->left->col = BLACK;
                    w->col = RED;
                    right_rotate(heap, w);
                    w = node->par_next->right;
                }

                w->col = node->par_next->col;
                node->par_next->col = BLACK;
                w->right->col = BLACK;
                left_rotate(heap, node->par_next);
                node = heap->free_list;
            }
        }

        else
        {
            w = node->par_next->left;
            if(w->col == RED)
            {
                w->col = BLACK;
                node->par_next->col = RED;
                right_rotate(heap, node->par_next);
                w = node->par_next->left;
            }

            if(w->right->col == BLACK && w->left->col == BLACK)
            {
                w->col = RED;
                node = node->par_next;
            }

            else
            {
                if(w->left->col == BLACK)
                {
                    w->right->col = BLACK;
                    w->col = RED;
                    left_rotate(heap, w);
                    w = node->par_next->left;
                }

                w->col = node->par_next->col;
                node->par_next->col = BLACK;
                w->left->col = BLACK;
                right_rotate(heap, node->par_next);
                node = heap->free_list;
            }
        }
    }

    node->col = BLACK;
}

void pop_block(M_Heap *heap, M_Block *node)
{
    M_Block *x = 0;
    M_Block *y = node;
    Color y_og_col = y->col;
    if(node->left == heap->t_nil)
    {
        x = node->right;
        rb_transplant(heap, node, node->right);
    }

    else if(node->right == heap->t_nil)
    {
        x = node->left;
        rb_transplant(heap, node, node->right);
    }

    else
    {
        y = rb_minimum(heap, node->right);
        y_og_col = y->col;
        x = y->right;
        if(y != node->right)
        {
            rb_transplant(heap, y, y->right);
            y->right = node->right;
            y->right->par_next = y;
        }

        else x->par_next = y;
        rb_transplant(heap, node, y);
        y->left = node->left;
        y->left->par_next = y;
        y->col = node->col;
    }

    if(y_og_col == BLACK) pop_fixup(heap, x);
}

M_Heap *init_heap(ptrdiff_t reserve, ptrdiff_t commit)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    ptrdiff_t page_size = sys_info.dwPageSize;
    reserve = align_pow_2(reserve, DEFAULT_ALIGN);
    commit = align_pow_2(commit, DEFAULT_ALIGN);

    void *base = VirtualAlloc(0, reserve, MEM_RESERVE, PAGE_READWRITE);
    VirtualAlloc(base, commit, MEM_COMMIT, PAGE_READWRITE);
    
    M_Block *t_nil = VirtualAlloc(0, sizeof(M_Block), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    M_Heap *heap = (M_Heap *)base;
    heap->capacity = reserve;
    heap->committed = commit;
    heap->offset = sizeof(M_Heap);

    heap->free_list = t_nil;
    heap->t_nil = t_nil;

    return heap;
}

void *heap_push_align(M_Heap *heap, ptrdiff_t size, ptrdiff_t align)
{
    M_Block *best = heap->free_list;
    for(M_Block *current = heap->free_list; current != heap->t_nil;)
    {
        if(current->size >= size)
        {
            best = current;
            current = current->left;
            continue;
        }
        current = current->right;
    }

    if(best != heap->t_nil)
    {
        pop_block(heap, best);
        return best;
    }
    
    ptrdiff_t padding = calc_padding_with_header(heap->offset, align, sizeof(M_Block));

    if(heap->offset + padding + size > heap->capacity) return 0;
    
    if(heap->offset + padding + size >= heap->committed)
    {
        ptrdiff_t cmt = 2 * align_pow_2(heap->offset + padding + size, DEFAULT_ALIGN);
        if(cmt >= heap->capacity) return 0;
        VirtualAlloc(heap, cmt, MEM_COMMIT, PAGE_READWRITE);
        heap->committed = cmt;
    }

    uintptr_t par_next_addr = (uintptr_t)heap + (uintptr_t) + heap->offset + (uintptr_t)padding;
    heap->offset += padding + size;

    M_Block *header = (M_Block *)(par_next_addr - sizeof(M_Block));
    header->size = size;

    return (void *)par_next_addr;
}

void *heap_zpush_align(M_Heap *heap, ptrdiff_t size, ptrdiff_t align)
{
    void *mem = heap_push_align(heap, size, align);
    __stosb(mem, 0, size);
    return mem;
}

void insert_fixup(M_Heap *heap, M_Block *header)
{
    M_Block *w = 0;
    for(; header->par_next->col == RED;)
    {
        if(header->par_next == header->par_next->par_next->left)
        {
            w = header->par_next->par_next->right;
            if(w->col == RED)
            {
                w->col = BLACK;
                header->par_next->col = BLACK;
                header->par_next->par_next->col = RED;
                header = header->par_next->par_next;
            }

            else
            {
                if(header == header->par_next->right)
                {
                    header = header->par_next;
                    left_rotate(heap, header);
                }

                header->par_next->col = BLACK;
                header->par_next->par_next->col = RED;
                right_rotate(heap, header->par_next->par_next);
            }
        }

        else
        {
            w = header->par_next->par_next->left;
            if(w->col == RED)
            {
                w->col = BLACK;
                header->par_next->col = BLACK;
                header->par_next->par_next->col = RED;
                header = header->par_next->par_next;
            }

            else
            {
                if(header == header->par_next->left)
                {
                    header = header->par_next;
                    right_rotate(heap, header);
                }

                header->par_next->col = BLACK;
                header->par_next->par_next->col = RED;
                left_rotate(heap, header->par_next->par_next);
            }
        }
    }

    heap->free_list->col = BLACK;
}

void heap_free(M_Heap *heap, void *block)
{
    M_Block *header = (M_Block *)((uintptr_t)block - sizeof(M_Block));

    M_Block *y = heap->t_nil;
    for(M_Block *x = heap->free_list; x != heap->t_nil;)
    {
        y = x;
        if(header->size < x->size)
        {
            x = x->left;
            continue;
        }
        x = x->right;
    }

    header->par_next = y;
    if(y == heap->t_nil)
    {
        heap->free_list = header;
    }

    else if(header->size < y->size)
    {
        y->left = header;
    }

    else y->right = header;

    header->left = heap->t_nil;
    header->right = heap->t_nil;
    header->col = RED;
    insert_fixup(heap, header);
}

void heap_clear_all(M_Heap *heap)
{
    heap->free_list = heap->t_nil;
    heap->offset = sizeof(M_Heap);
}

TempHeap begin_temp(M_Heap *heap)
{
    TempHeap tmp = {.heap = heap, .chain = 0};
    return tmp;
}

void *tmp_push_align(TempHeap *tmp, ptrdiff_t size, ptrdiff_t align)
{
    void *mem = heap_push_align(tmp->heap, size, align);
    M_Block *header = (M_Block *)((uintptr_t)mem - sizeof(M_Block));
    header->par_next = tmp->chain;
    tmp->chain = header;
    return mem;
}

void *tmp_zpush_align(TempHeap *tmp, ptrdiff_t size, ptrdiff_t align)
{
    void *mem = tmp_push_align(tmp, size, align);
    __stosb(mem, 0, size);
    return mem;
}

void end_temp(TempHeap *tmp)
{
    M_Block *block = tmp->chain;
    for(M_Block *block = tmp->chain; block;)
    {
        M_Block *par_next = block->par_next;
        void *mem = (void *)((uintptr_t)block + sizeof(M_Block));
        heap_free(tmp->heap, mem);
        block = par_next;
    }
}
