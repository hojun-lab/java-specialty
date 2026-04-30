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

void simulate_oom(Heap *heap) {
    printf("=== OOM 시뮬레이션 ===\n");
    int count = 0;
    while (1) {
        Object *obj = heap_alloc(heap, 64);
        if (obj == NULL) {
            printf("OOM! objects = %d heap_alloc failed\n", count);
            printf("Heap usage: %dB / %dB\n", heap->used, heap->total_size);
            break;
        }
        count++;
    }
}

void simulate_leak(Heap *heap) {
    printf("=== 메모리 누수 시뮬레이션 ===\n");

    Object *cache[100];     // 누수의 원인 -> 참조를 계속 붙잡음
    int cache_count = 0;

    for (int i = 0; i < 5; i++) {
        Object *obj = heap_alloc(heap, 32);
        cache[cache_count++] = obj;     // <- GC 해도 살아남
        printf("할당 : [%p] 누적=%dB\n", (void*)obj, heap->used);
    }

    printf("GC 돌려도 cache[]가 참조 중 -> 수집 안 됨\n");
    printf("힙 사용량: %dB / %dB\n", heap->used, heap->total_size);
}

void diagnose(Heap *heap) {
    printf("\n=== 진단 시작===\n");

    // 1. 힙 사용률 체크
    int usage_percent = (heap->used * 100) / heap->total_size;
    printf("[MONITOR] 힙 사용률 : %d%%\n", usage_percent);

    if (usage_percent >= 90) {
        printf("  ⚠ 경고: 힙 90%% 초과\n");
        printf("  → 원인: 메모리 누수 또는 힙 크기 부족\n");
        printf("  → 확인: heap_dump()로 객체 분포 확인\n");
        printf("  → 해결: -Xmx 증가 또는 누수 코드 수정\n");
    }

    // 2. 객체 분포 분석
    char *ptr = heap->start;
    int obj_count = 0;
    int old_objects = 0;    // age >= 3

    while (ptr < heap->alloc_ptr) {
        Object *obj = (Object *)ptr;
        obj_count++;
        if (obj->gc_age >= 3) {
            old_objects++;
        }
        ptr += obj->size;
    }

    printf("객체 총 %d개, Old 승격 %d개\n", obj_count, old_objects);

    if (old_objects > obj_count / 2) {
        printf("  ⚠ 경고: Old Gen 객체 비율 높음\n");
        printf("  → 원인: 객체 수명이 길거나 누수 의심\n");
        printf("  → 확인: jmap -histo로 클래스별 집계\n");
    }

    printf("=== 진단 완료 ===\n");
}

int main(void) {
    Heap heap;
    heap_init(&heap, 1024);

    // heap_alloc(&heap, 32);
    // heap_alloc(&heap, 64);
    // heap_alloc(&heap, 16);
    //
    // printf("=== Heap Dump ===\n");
    // heap_dump(&heap);
    // printf("=================\n");

    // simulate_oom(&heap);
    //
    // printf("===힙 덤프===\n");
    // heap_dump(&heap);

    // OOM 시뮬레이션
    simulate_oom(&heap);
    heap_dump(&heap);
    diagnose(&heap);

    printf("\n");

    Heap heap2;
    heap_init(&heap2, 1024);
    simulate_leak(&heap2);
    heap_dump(&heap2);
    diagnose(&heap2);

    return 0;
}