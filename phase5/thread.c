#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#define MAX_QUEUE   32
#define MAX_WORKERS 8

int thread_count = 0;

typedef enum
{
    THREAD_NEW,
    THREAD_RUNNABLE,
    THREAD_BLOCKED,
    THREAD_WAITING,
    THREAD_TERMINATED
} ThreadState;

typedef struct {
    void *(*func)(void *); // 실행할 함수
    void *arg;             // 함수에 넘길 인자
} Task;

const char *state_to_string(ThreadState s) {
    switch (s) {
        case THREAD_NEW: return "NEW";
        case THREAD_RUNNABLE: return "RUNNABLE";
        case THREAD_BLOCKED: return "BLOCKED";
        case THREAD_WAITING: return "WAITING";
        case THREAD_TERMINATED: return "TERMINATED";
        default:                return "UNKNOWN";
    }
}

typedef struct Monitor Monitor;
typedef struct JVMThread JVMThread;

struct Monitor
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    JVMThread *owner;
    int recursion_count;
    JVMThread *waiting_threads[8]; // lock을 기다리는 스레드들
    int waiting_count;
};

struct JVMThread
{
    pthread_t pthread;
    char name[64];
    ThreadState state;
    int id;
    Monitor *waiting_for;
};

typedef struct {
    Task queue[MAX_QUEUE];
    int queue_size;
    JVMThread *workers[MAX_WORKERS];
    int workers_count;
    Monitor monitor;
    int running;
} ThreadPool;

typedef struct {
    ThreadPool *pool;
    JVMThread *self;
} WorkerArg;

JVMThread *all_threads[64];

JVMThread *jvm_thread_create(const char *name, void *(*func)(void *), void *arg)
{
    JVMThread *thread = malloc(sizeof(JVMThread));
    strcpy(thread->name, name);
    thread->id = thread_count;
    all_threads[thread_count] = thread;
    thread_count++;
    thread->waiting_for = NULL;
    thread->state = THREAD_NEW;

    pthread_create(&thread->pthread, NULL, func, arg);
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

void monitor_init(Monitor *mon)
{
    pthread_mutex_init(&mon->mutex, NULL); // 초기화
    pthread_cond_init(&mon->cond, NULL);
    mon->owner = NULL;
    mon->recursion_count = 0;
}

void monitor_lock(Monitor *mon, JVMThread *thread)
{
    printf("[%s] waiting for lock...\n", thread->name);
    thread->state = THREAD_BLOCKED;                      // 대기 상태
    // mon->waiting_threads[mon->waiting_count++] = thread; // 스레드 등록

    thread->waiting_for = mon;
    pthread_mutex_lock(&mon->mutex); // 대기 실행 로직
    thread->waiting_for = NULL;

    // mon->waiting_count--;            // 획득 했으니 대기 목록에서 제거
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

void monitor_wait(Monitor *mon, JVMThread *thread)
{
    thread->state = THREAD_WAITING;
    mon->owner = NULL;
    printf("[%s] waiting...\n", thread->name);
    pthread_cond_wait(&mon->cond, &mon->mutex);
    mon->owner = thread;
    thread->state = THREAD_RUNNABLE;
    printf("[%s] woke up!\n", thread->name);
}

void monitor_notify(Monitor *mon, JVMThread *thread)
{
    pthread_cond_signal(&mon->cond);
}

Monitor lock;
Monitor lockA;
Monitor lockB;

void *worker(void *arg)
{
    JVMThread *self = (JVMThread *)arg;

    monitor_lock(&lock, self);
    printf("[%s] id=%d, state=%d, running!\n", self->name, self->id, self->state);
    sleep(1);
    monitor_unlock(&lock, self);

    return NULL;
}

void *worker1(void *arg)
{
    JVMThread *self = (JVMThread *)arg;
    monitor_lock(&lockA, self);
    printf("[%s] got lockA, trying lockB...\n", self->name);
    sleep(1);
    monitor_lock(&lockB, self);
    monitor_unlock(&lockB, self);
    monitor_unlock(&lockA, self);
    return NULL;
}

void *worker2(void *arg)
{
    JVMThread *self = (JVMThread *)arg;
    monitor_lock(&lockB, self);
    printf("[%s] got lockB, trying lockA...\n", self->name);
    sleep(1);
    monitor_lock(&lockA, self);
    monitor_unlock(&lockB, self);
    monitor_unlock(&lockA, self);
    return NULL;
}

void *deadlock_detector(void *arg)
{
    sleep(3);
    printf("\n=== Deadlock Detection ===\n");
    for (int i = 0; i < thread_count; i++)
    {
        JVMThread *t = all_threads[i];
        if (t->waiting_for == NULL)
            continue;
        JVMThread *current = t;
        for (int step = 0; step < thread_count; step++)
        {
            Monitor *mon = current->waiting_for;
            if (mon == NULL || mon->owner == NULL)
                break;

            current = mon->owner;

            if (current == t)
            {
                printf("DEADLOCK DETECTED!\n");
                printf("  %s → %s → cycle!\n", t->name, current->name);
                return NULL;
            }
        }
    }

    printf("No deadlock found.\n");
    return NULL;
}

// int main(void)
// {
//     // monitor_init(&lock);
//
//     // JVMThread *t1 = jvm_thread_create("http-handler-1", worker);
//     // JVMThread *t2 = jvm_thread_create("http-handler-2", worker);
//
//     // pthread_join(t1->pthread, NULL);
//     // pthread_join(t2->pthread, NULL);
//
//     // printf("\nAll threads finished.\n");
//     // return 0;
//
//     monitor_init(&lockA);
//     monitor_init(&lockB);
//
//     JVMThread *t1 = jvm_thread_create("Thread-1", worker1);
//     JVMThread *t2 = jvm_thread_create("Thread-2", worker2);
//     JVMThread *detector = jvm_thread_create("detector", deadlock_detector);
//
//     pthread_join(t1->pthread, NULL);
//     pthread_join(t2->pthread, NULL);
//     pthread_join(detector->pthread, NULL);
//
//     printf("ALL THREADS FINISHED\n");
//     return 0;
// }

Monitor queue_lock;
int data_ready = 0;

void* consumer(void *arg) {
    JVMThread *self = (JVMThread *)arg;
    monitor_lock(&queue_lock, self);

    while (!data_ready) {
        printf("[%s] no data, waiting...\n", self->name);
        monitor_wait(&queue_lock, self);
    }

    printf("[%s] got data! processing!\n", self->name);
    monitor_unlock(&queue_lock, self);
    self->state = THREAD_TERMINATED;
    return NULL;
}

void* producer(void *arg) {
    JVMThread *self = (JVMThread *)arg;
    sleep(2);

    monitor_lock(&queue_lock, self);
    data_ready = 1;
    printf("[%s] data produced! notifying...\n", self->name);
    monitor_unlock(&queue_lock, self);
    monitor_notify(&queue_lock, self);
    self->state = THREAD_TERMINATED;
    return NULL;
}

void *pool_worker(void *arg) {
    WorkerArg *warg = (WorkerArg *)arg;
    ThreadPool *pool = warg->pool;

    JVMThread *self = NULL;
    pthread_t me = pthread_self();
    for (int i = 0; i < thread_count; i++) {
        if (pthread_equal(all_threads[i]->pthread, me)) {
            self = all_threads[i];
            break;
        }
    }
    while (1) {
        monitor_lock(&pool->monitor, self);

        // 큐가 비어 있고 실행 중이면 -> 잠들기
        while (pool->queue_size == 0 && pool->running) {
            monitor_wait(&pool->monitor, self);
        }

        // running = 0 이 큐도 비면 -> 종료
        if (!pool->running && pool->queue_size == 0) {
            monitor_unlock(&pool->monitor, self);
            break;
        }

        // 큐에서 Task 꺼내기
        Task task = pool->queue[pool->queue_size - 1];
        pool->queue_size--;

        monitor_unlock(&pool->monitor, self);

        task.func(task.arg);
    }
    return NULL;
}

void threadpool_init(ThreadPool *pool, int worker_count) {
    monitor_init(&pool->monitor);
    pool->queue_size = 0;
    pool->workers_count = worker_count;
    pool->running = 1;

    for (int i = 0; i < worker_count; i++) {
        char name[32];
        sprintf(name, "worker-%d", i);

        WorkerArg *warg = malloc(sizeof(WorkerArg));
        warg->pool = pool;

        pool->workers[i] = jvm_thread_create(name, pool_worker, warg);
        warg->self = pool->workers[i];
    }
}

void threadpool_submit(ThreadPool *pool, void *(*func)(void *), void *arg) {
    pthread_mutex_lock(&pool->monitor.mutex);

    Task t;
    t.func = func;
    t.arg = arg;
    pool->queue[pool->queue_size] = t;
    pool->queue_size++;

    pthread_mutex_unlock(&pool->monitor.mutex);
    pthread_cond_signal(&pool->monitor.cond);
}

void thread_dump() {
    printf("=== Thread Dump ===\n");
    for (int i = 0; i < thread_count; i++) {
        JVMThread *t = all_threads[i];
        printf("\"%s\" id = %d state = %s\n", t->name, t->id, state_to_string(t->state));
        if (t->waiting_for != NULL) {
        printf("  waiting for monitor\n");
        }
    }
    printf("===================\n");
}

void *my_task(void *arg) {
    int num = *(int *)arg;
    printf("Task %d 실행 중\n", num);
    sleep(1);
    printf("Task %d 완료\n", num);
    return NULL;
}

int main(void) {
    // monitor_init(&queue_lock);
    //
    // JVMThread *c = jvm_thread_create("consumer", consumer);
    // JVMThread *p = jvm_thread_create("producer", producer);
    //
    // pthread_join(c->pthread, NULL);
    // pthread_join(p->pthread, NULL);
    //
    // printf("All done!\n");
    ThreadPool pool;
    threadpool_init(&pool, 3);

    int arg[5] = { 1, 2, 3, 4, 5 };
    for (int i = 0; i < 5; i++) {
        threadpool_submit(&pool, my_task, &arg[i]);
    }

    sleep(5);
    thread_dump();
    return 0;
}
