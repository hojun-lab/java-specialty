#define COMPILE_THRESHOLD 5
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef struct {
    char name[64];
    int call_count;  // 호출 횟수
    int is_hot;      // 1 JIT 컴파일러 대상
} Method;

void method_call(Method *m) {
    m->call_count++;

    if (m->call_count >= COMPILE_THRESHOLD && !m->is_hot) {
        m->is_hot = 1;
        printf("[JIT] %s is HOT! compiling...\n", m->name);
    }

    printf("[%s] call %d (%s)\n",
        m->name,
        m->call_count,
        m->is_hot ? "JIT" : "interpreted");
}

void *alloc_executable(int size) {
    void *mem = mmap(
        NULL,                                   // 주소는 OS가 결정
        size,                                   // 크기
        PROT_READ | PROT_WRITE,                 // 읽기 + 쓰기
        MAP_PRIVATE | MAP_ANON,                 // 파일 없이 메모리만
        -1, 0
    );
    if (mem == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }
    return mem;
}

void make_executable(void *mem, int size) {
    mprotect(mem, size, PROT_READ | PROT_EXEC);
}

int main(void) {
    Method m;
    strcpy(m.name, "add");
    m.call_count = 0;
    m.is_hot = 0;

    for (int i = 0; i < 7; i++) {
        method_call(&m);
    }

    void *mem = alloc_executable(4096);
    printf("실행 가능한 메모리 주소 : %p\n", mem);

    return 0;
}