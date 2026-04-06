#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

int thread_count = 0;

typedef enum
{
    THREAD_NEW,
    THREAD_RUNNABLE,
    THREAD_BLOCKED,
    THREAD_WAITING,
    THREAD_TERMINATED
} ThreadState;

typedef struct
{
    pthread_t pthread;
    char name[64];
    ThreadState state;
    int id;
} JVMThread;

JVMThread *jvm_thread_create(const char *name, void *(*func)(void *))
{
    JVMThread *thread = malloc(sizeof(JVMThread));
    strcpy(thread->name, name);
    thread->id = thread_count++;
    thread->state = THREAD_NEW;

    pthread_create(&thread->pthread, NULL, func, thread);
    thread->state = THREAD_RUNNABLE;

    return thread;
}

void *thread_function(void *arg)
{
    // new Thread(()->{
    //     System.out.println("Hello from thread!");
    // }).start();

    printf("Hello from thread!\n");
    return NULL;
}

typedef struct
{
    pthread_mutex_t mutex;
    JVMThread *owner;
    int recursion_count;
} Monitor;

void monitor_init(Monitor *mon)
{
    pthread_mutex_init(&mon->mutex, NULL); // 초기화
    mon->owner = NULL;
    mon->recursion_count = 0;
}

void monitor_lock(Monitor *mon, JVMThread *thread)
{
    printf("[%s] waiting for lock...\n", thread->name);
    thread->state = THREAD_BLOCKED;  // 대기 상태
    pthread_mutex_lock(&mon->mutex); // 대기 실행 로직
    thread->state = THREAD_RUNNABLE; // 획득
    mon->owner = thread;             // 주인 설정
    printf("[%s] acquired lock!\n", thread->name);
}

void monitor_unlock(Monitor *mon, JVMThread *thread)
{
    printf("[%s] release lock...\n", thread->name);
    mon->owner = NULL;                 // 주인 해제
    pthread_mutex_unlock(&mon->mutex); // 풀기
}

Monitor lock;

void *worker(void *arg)
{
    JVMThread *self = (JVMThread *)arg;

    monitor_lock(&lock, self);
    printf("[%s] id=%d, state=%d, running!\n", self->name, self->id, self->state);
    sleep(1);
    monitor_unlock(&lock, self);

    return NULL;
}

int main(void)
{
    monitor_init(&lock);

    JVMThread *t1 = jvm_thread_create("http-handler-1", worker);
    JVMThread *t2 = jvm_thread_create("http-handler-2", worker);

    pthread_join(t1->pthread, NULL);
    pthread_join(t2->pthread, NULL);

    printf("\nAll threads finished.\n");
    return 0;
}
