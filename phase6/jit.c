#define COMPILE_THRESHOLD 5
#include <stdio.h>
#include <string.h>

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

int main(void) {
    Method m;
    strcpy(m.name, "add");
    m.call_count = 0;
    m.is_hot = 0;

    for (int i = 0; i < 7; i++) {
        method_call(&m);
    }

    return 0;
}