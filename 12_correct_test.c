#include "uthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>


const unsigned long kArgF = 111;
const unsigned long kArgG = 222;
const int ans_pid_rep[] = {1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 0};
int thread_pids[1000];
int cur_id = 0;

void g(void* data)
{
    assert(data == (void*)kArgG);

    static const int pid = 2;
    thread_pids[cur_id++] = pid;

    //printf("\n%lu %p %p %p\n", fiber_count, current->prev, current, current->next);
    printf("In g(), data=%lx %d\n", (unsigned long) data, cur_id);
    
}

void f(void* data)
{
    assert(data == (void*)kArgF);

    static const int pid = 1;
    thread_pids[cur_id++] = pid;
    for (int i = 0; i < 3; ++i) {
        //printf("\n%lu %p %p %p\n", fiber_count, current->prev, current, current->next);
        printf("In f(), i=%d, data=%lx %d\n", i, (unsigned long) data, cur_id);
        uthread_start(g, (void*)kArgG);
        uthread_yield();
        thread_pids[cur_id++] = pid;
    }
}

void check_pids()
{
    printf("cur_id = %d\n", cur_id);
    assert(cur_id == 14);
    for (int i = 0; i < cur_id; i++) {
        assert(thread_pids[i] == ans_pid_rep[i]);
    }
}




int main()
{
    static const int pid = 0;
    //printf("\n%lu %p %p %p\n", fiber_count, current->prev, current, current->next);
    printf("In main() %d\n", cur_id);
    uthread_start(f, (void*) kArgF);
    thread_pids[cur_id++] = pid;
    while (!uthread_try_join()) {
        //printf("\n%lu %p %p %p\n", fiber_count, current->prev, current, current->next);
        printf("In main() %d\n", cur_id);
        uthread_yield();
        thread_pids[cur_id++] = pid;
    }
    check_pids();
    return 0;
}
