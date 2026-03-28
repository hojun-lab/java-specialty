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

void object_add_ref(Object *from, Object *to)
{
    from->refs[from->ref_count] = to;
    from->ref_count++;
}

void gc_mark(Object *obj)
{
    if (obj->gc_marked)
        return;

    obj->gc_marked = 1;
    for (int i = 0; i < obj->ref_count; i++)
    {
        gc_mark(obj->refs[i]);
    }
}

void gc_sweep(Heap *heap)
{
    char *ptr = heap->start;
    while (ptr < heap->alloc_ptr)
    {
        Object *obj = (Object *)ptr;
        if (obj->gc_marked)
        {
            // 살아있음 -> 초기화
            obj->gc_marked = 0;
        }
        else
        {
            printf("  [SWEEP] collected object at %p (size=%d)\n", ptr, obj->size);
        }
        ptr = ptr + obj->size;
    }
}

int main(void)
{
    Heap heap;
    heap_init(&heap, 1024);

    Object *a = heap_alloc(&heap, 32);
    Object *b = heap_alloc(&heap, 32);
    Object *c = heap_alloc(&heap, 32);
    Object *d = heap_alloc(&heap, 32); // 아무도 참조 안 하는 객체!

    object_add_ref(a, b); // a → b
    object_add_ref(a, c); // a → c

    printf("a refs: %d\n", a->ref_count); // 2
    printf("b refs: %d\n", b->ref_count); // 0
    printf("c refs: %d\n", c->ref_count); // 0

    gc_mark(a); // a에서 시작해서 b, c까지 마킹

    printf("a marked: %d\n", a->gc_marked); // 1
    printf("b marked: %d\n", b->gc_marked); // 1
    printf("c marked: %d\n", c->gc_marked); // 1

    object_add_ref(a, b);
    object_add_ref(a, c);
    // d는 참조 안 함!

    printf("=== GC Start ===\n");
    gc_mark(a);      // a, b, c만 마킹됨
    gc_sweep(&heap); // d는 마킹 안 됨 → 수거!
    printf("=== GC End ===\n");

    return 0;
}