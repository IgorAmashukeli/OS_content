#include "uthread.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


#define STACK_FRAME_SIZE 8192
#define REG_NUM 10



/**malloc with error checking**/
#define MALLOC(type, count, res)\
do {\
    void* ptr = malloc(count * sizeof(type));\
    if (ptr == NULL) {\
        perror("malloc");\
        exit(EXIT_FAILURE);\
    }\
    res = (type*) ptr;\
} while(false)\




/**Context list definition**/
typedef struct Context {
    // stack pointer
    void* rsp;

    // stack start
    void* stack_start;

    // is context deleted or not
    bool is_deleted;

    // next context
    struct Context* next;

    // previous context
    struct Context* prev;
    
} Context;


/**uthread context switch from asm file**/
extern void uthread_switch(Context *old, Context *current);


/**uthread prepare to handler from asm file**/
extern void uthread_prepare(Context* ctx, void(*uthread_handle)(void(*func)(void*), void*),void (*f) (void*),
 void* data);



/**initial Context is stored in global memory**/
Context initial = {NULL, NULL, false, &initial, &initial};


/**head, tail, current context variables**/
Context* current = &initial;
Context* head = &initial;
Context* tail = &initial;


/**number of fibers now working; at first it is 1**/
size_t fiber_count = 1;



/**

1) returns 1 if the current fiber is the only uthread in the program
2) returns 0 otherwise

**/
int uthread_try_join() {
    // only one current fiber
    if (fiber_count == 1) {
        return 1;
    }

    // more than one current fiber
    return 0;
}




/**
extract Context from Context list

This will be called only with >= 2 elements in the list
**/

void extract(Context* ctx) {

    // if there are two elements in the list
    if (ctx->prev == ctx->next) {
        Context* last_one = ctx->prev;
        last_one->next = last_one;
        last_one->prev = last_one;

        head = last_one;
        tail = last_one;

        return;
    }


    // if there are more elements in the list:


    // we change head/tail to the next/prev element
    if (head == ctx) {
        head = ctx->next;
    } else if (tail == ctx) {
        tail = ctx->prev;
    }

    
    // we connect prev with next
    (ctx->prev)->next = ctx->next;
    (ctx->next)->prev = ctx->prev;
}



/**
Insert Context to the Context list

This will be called only with >= 1 elements in the list
**/
void insert(Context* ctx) {


    // insert in list
    tail->next = ctx;
    ctx->prev = tail;

    ctx->next = head;
    head->prev = ctx;

    tail = ctx;

}



/**
delete Context

1) extract Context from Context list

2) free stack

3) free Context

4) decrease number of working fibers

**/
void delete(Context* ctx) {
    // extract from list
    extract(ctx);

    // free stack
    free(ctx->stack_start);

    // free context
    free(ctx);

    // decrease number of fibers
    --fiber_count;
}





/**does yield**/
void uthread_yield() {
    
    // saves current context as old context
    Context* old = current;

    // move current to the next
    current = current->next;

    // do uthread switch
    uthread_switch(old, current);  

    // if previous is marked "deleted", delete it
    Context* previous = current->prev;
    if (previous->is_deleted) {
        delete(previous);
    }
}



/**handler for function**/
void uthread_handle(void (*f) (void*), void* data) {


    // call function
    f(data);

    // function ended
    current->is_deleted = true;

    // yield to the next function
    uthread_yield();
}


/**create new context for a new fiber**/
Context* create() {
    // allocate context
    Context* ctx;
    MALLOC(Context, 1, ctx);

    // allocate stack
    MALLOC(void, STACK_FRAME_SIZE, ctx->stack_start);

    // look explanation in uthread_prepare
    ctx->rsp = ctx->stack_start + STACK_FRAME_SIZE - REG_NUM * 8;

    // context is not deleted
    ctx->is_deleted = false;

    // insert current context to the list
    insert(ctx);

    // increment fiber count
    ++fiber_count;

    return ctx;
}



/**starts new fiber**/
void uthread_start(void (*f)(void*), void* data) {

    // create new context
    Context* ctx = create();


    // prepare for switch context
    uthread_prepare(ctx, uthread_handle, f, data);

    // yield
    uthread_yield();
}
