#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

void gc_report(int eden_before, int eden_after, int eden_total,
               int s_before, int s_after, int s_total,
               int old_before, int old_after, int old_total,
               int promoted, int collected, double elapsed_ms)
{
    printf("[GC (Allocation Failure) %.4fms]\n", elapsed_ms);
    printf("  [Eden: %dB->%dB(%dB)]\n", eden_before, eden_after, eden_total);
    printf("  [Survivor: %dB->%dB(%dB)]\n", s_before, s_after, s_total);
    printf("  [Old: %dB->%dB(%dB)]\n", old_before, old_after, old_total);
    printf("  [Promoted: %dB] [Collected: %dB]\n", promoted, collected);
}

void minor_gc(Heap *eden, Heap *from, Heap *to, Heap *old_gen,
              Object **roots, int root_count)
{
    clock_t start = clock();
    int eden_before = eden->used;
    int s_before = from->used;
    int old_before = old_gen->used;

    printf("\n=== Minor GC Start ===\n");
    for (int i = 0; i < root_count; i++)
    {
        gc_mark(roots[i]);
    }

    char *ptr = eden->start;
    while (ptr < eden->alloc_ptr)
    {
        Object *obj = (Object *)ptr;

        int tenuring_threshold = 3;
        if (obj->gc_marked)
        {
            Object *new_obj;
            if (obj->gc_age + 1 >= tenuring_threshold)
            {
                new_obj = heap_alloc(old_gen, obj->size - sizeof(Object));
                printf("  [PROMOTE] age=%d → Old Gen\n", obj->gc_age + 1);
            }
            else
            {
                new_obj = heap_alloc(to, obj->size - sizeof(Object));
                if (new_obj == NULL)
                {
                    new_obj = heap_alloc(old_gen, obj->size - sizeof(Object));
                    printf("  [PROMOTE] Survivor full! → Old Gen\n");
                }
                else
                {
                    printf("  [COPY] age=%d → Survivor\n", obj->gc_age + 1);
                }
            }

            memcpy(new_obj, obj, obj->size);
            new_obj->gc_marked = 0;
            new_obj->gc_age++;

            for (int r = 0; r < root_count; r++)
            {
                if (roots[r] == obj)
                {
                    roots[r] = new_obj;
                }
            }

            printf("  [COPY] age=%d, size=%d → Survivor\n", new_obj->gc_age, obj->size);
        }
        ptr = ptr + obj->size;
    }

    ptr = from->start;
    while (ptr < from->alloc_ptr)
    {
        Object *obj = (Object *)ptr;

        int tenuring_threshold = 3;
        if (obj->gc_marked)
        {
            Object *new_obj;
            if (obj->gc_age + 1 >= tenuring_threshold)
            {
                new_obj = heap_alloc(old_gen, obj->size - sizeof(Object));
                printf("  [PROMOTE] age=%d → Old Gen\n", obj->gc_age + 1);
            }
            else
            {
                new_obj = heap_alloc(to, obj->size - sizeof(Object));
                if (new_obj == NULL)
                {
                    new_obj = heap_alloc(old_gen, obj->size - sizeof(Object));
                    printf("  [PROMOTE] Survivor full! → Old Gen\n");
                }
                else
                {
                    printf("  [COPY] age=%d → Survivor\n", obj->gc_age + 1);
                }
            }

            memcpy(new_obj, obj, obj->size);
            new_obj->gc_marked = 0;
            new_obj->gc_age++;

            for (int r = 0; r < root_count; r++)
            {
                if (roots[r] == obj)
                {
                    roots[r] = new_obj;
                }
            }

            printf("  [COPY] age=%d, size=%d → Survivor\n", new_obj->gc_age, obj->size);
        }
        ptr = ptr + obj->size;
    }

    eden->alloc_ptr = eden->start;
    eden->used = 0;
    from->alloc_ptr = from->start;
    from->used = 0;

    printf("=== Minor GC End ===\n");

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    int collected = eden_before + s_before - to->used - (old_gen->used - old_before);

    gc_report(
        eden_before, eden->used, eden->total_size,
        s_before, to->used, to->total_size,
        old_before, old_gen->used, old_gen->total_size,
        old_gen->used - old_before,
        collected,
        elapsed);
}

int main(void)
{
    Heap eden, survivor0, survivor1, old_gen;
    heap_init(&eden, 512);
    heap_init(&survivor0, 256);
    heap_init(&survivor1, 256);
    heap_init(&old_gen, 512);

    Heap *from = &survivor0;
    Heap *to = &survivor1;

    // Eden 에 객체 할당
    Object *a = heap_alloc(&eden, 32);
    Object *b = heap_alloc(&eden, 32);
    Object *c = heap_alloc(&eden, 32); // 참조 안됨 -> GC 대상

    object_add_ref(a, b);

    Object *roots[] = {a};
    for (int i = 0; i < 3; i++)
    {
        minor_gc(&eden, from, to, &old_gen, roots, 1);
        Heap *temp = from;
        from = to;
        to = temp;
    }

    printf("=== Before Minor GC ===\n");
    printf("Eden used: %d / %d\n", eden.used, eden.total_size);
    printf("Survivor0 used: %d / %d\n", survivor0.used, survivor0.total_size);
    printf("Survivor1 used: %d / %d\n", survivor1.used, survivor1.total_size);
    printf("Old used: %d / %d\n", old_gen.used, old_gen.total_size);

    return 0;
}