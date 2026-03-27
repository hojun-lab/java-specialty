#include <stdio.h>
#include <stdlib.h>
typedef struct
{
    char *start;
    char *end;
    char *alloc_ptr;
    int total_size;
    int used;
} Heap;

void heap_init(Heap *heap, int size)
{
    char *memory = malloc(size);
    heap->start = memory;
    heap->alloc_ptr = memory;
    heap->end = memory + size;
    heap->total_size = size;
    heap->used = 0;
}

char *heap_alloc(Heap *heap, int size)
{
    if (heap->alloc_ptr + size > heap->end)
    {
        return NULL;
    }

    char *ptr = heap->alloc_ptr;
    heap->alloc_ptr = ptr + size;
    heap->used = heap->used + size;
    return ptr;
}

int main(void)
{
    Heap heap;
    heap_init(&heap, 1024);

    printf("=== Heap Created ===\n");
    printf("Total: %d bytes\n", heap.total_size);
    printf("Used: %d bytes\n\n", heap.used);

    char *obj1 = heap_alloc(&heap, 64);
    printf("Object 1 allocated: 64 bytes\n");
    printf("Used: %d / %d bytes\n\n", heap.used, heap.total_size);

    char *obj2 = heap_alloc(&heap, 128);
    printf("Object 2 allocated: 128 bytes\n");
    printf("Used: %d / %d bytes\n\n", heap.used, heap.total_size);

    char *obj3 = heap_alloc(&heap, 900);
    if (obj3 == NULL)
    {
        printf("OOM! 할당 실패\n");
    }
    else
    {
        printf("Object 3 allocated: 900 bytes\n");
        printf("Used: %d / %d bytes\n", heap.used, heap.total_size);
    }

    return 0;
}