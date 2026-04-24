#define COMPILE_THRESHOLD 5
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef struct {
    char name[64];
    int call_count;  // 호출 횟수
    int is_hot;      // 1 JIT 컴파일러 대상
    int (*compiled_fn)(int, int);
} Method;

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

void method_call(Method *m, int a, int b) {
    m->call_count++;

    if (m->call_count >= COMPILE_THRESHOLD && !m->is_hot) {
        m->is_hot = 1;
        printf("[JIT] %s is HOT! compiling...\n", m->name);

        unsigned char code[] = {
            0x00, 0x00, 0x01, 0x8B,
            0xC0, 0x03, 0x5F, 0xD6
        };

        void *mem = alloc_executable(4096);
        memcpy(mem, code, sizeof(code));
        make_executable(mem, 4096);

        m->compiled_fn = (int (*)(int, int)) mem;
    }

    if (m->is_hot) {
        int result = m->compiled_fn(a, b);
        printf("[%s] call %d (JIT) = %d\n", m->name, m->call_count, result);
    } else {
        printf("[%s] call %d (interpreted)\n", m->name, m->call_count);
    }
}

int main(void) {
    Method m;
    strcpy(m.name, "add");
    m.call_count = 0;
    m.is_hot = 0;

    for (int i = 0; i < 7; i++) {
        method_call(&m, 3, 4);
    }

    void *mem = alloc_executable(4096);
    printf("실행 가능한 메모리 주소 : %p\n", mem);

    unsigned char code[] = {
        0x00, 0x00, 0x01, 0x8B,
        0xC0, 0x03, 0x5F, 0xD6
    };

    memcpy(mem, code, sizeof(code));
    make_executable(mem, 4096);

    int (*jit_add)(int, int) = (int (*)(int, int)) mem;
    int result = jit_add(3, 4);
    printf("JIT add(3, 4) = %d\n", result);

    return 0;
}