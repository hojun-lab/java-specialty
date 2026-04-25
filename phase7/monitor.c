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

typedef struct Object
{
    int size;               // 객체의 전체 크기
    int gc_marked;          // GC의 마킹 여부 (0 or 1)
    int gc_age;             // GC 생존 횟수 (0~15)
    int ref_count;          // 참조횟수
    struct Object *refs[8]; // 참조배열
    char data[];            // 실제 데이터 (가변 크기)
} Object;

void heap_dump(Heap *heap) {
    char *ptr = heap->start;
    int obj_count = 0;
    int total_bytes = 0;

    while (ptr < heap->alloc_ptr) {
        const Object *obj = (Object *)ptr;
        printf("[%p] size=%dB age=%d refs=%d\n", (void *)obj, obj->size, obj->gc_age, obj->ref_count);
        obj_count++;
        total_bytes += obj->size;
        ptr = ptr + obj->size;
    }

    printf("Total : %d objects, %dB / %dB used\n", obj_count, total_bytes, heap->total_size);
}

void heap_init(Heap *heap, int size)
{
    char *memory = malloc(size);
    heap->start = memory;
    heap->alloc_ptr = memory;
    heap->end = memory + size;
    heap->total_size = size;
    heap->used = 0;
}

Object *heap_alloc(Heap *heap, int size)
{
    int total_size = sizeof(Object) + size;
    if (heap->alloc_ptr + total_size > heap->end)
    {
        return NULL;
    }

    char *ptr = heap->alloc_ptr;
    heap->alloc_ptr = ptr + total_size;
    heap->used = heap->used + total_size;

    Object *obj = (Object *)ptr;
    obj->size = total_size;
    obj->gc_marked = 0;
    obj->gc_age = 0;
    obj->ref_count = 0;
    return obj;
}

int main(void) {
    Heap heap;
    heap_init(&heap, 1024);

    heap_alloc(&heap, 32);
    heap_alloc(&heap, 64);
    heap_alloc(&heap, 16);

    printf("=== Heap Dump ===\n");
    heap_dump(&heap);
    printf("=================\n");
    return 0;
}