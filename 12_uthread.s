// your code goes here
// you may find these stubs useful:
    .global uthread_switch
    .global uthread_prepare




/**context switch

1) saves %rdi and %rsi
2) saves callee-saved registers (except %rsp) to the stack
3) saves old %rsp to the old context structure
4) loads current %rsp from current context structure
5) loads other callee-saved register (except % rsp) from the stack
6) loads %rdi and %rsi

**/


uthread_switch:
        // save old

        // saves %rdi and %rsi
        // because there is pop of %rdi and %rsi
        pushq %rdi
        pushq %rsi

        // saves callee-saved registers
        pushq %rbx
        pushq %rbp
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15
        // save rsp to the old context structure
        // (%rdi) is "void* rsp"
        movq %rsp, (%rdi)


        // load current

        // load current "rsp" to the current context structure
        movq (%rsi), %rsp
        // loads callee-saved registers
        popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %rbp
        popq %rbx

        // loads %rdi and %si
        // because this is moved to the stack by uthread_jmp
        // to have arguments for uthread_handle
        popq %rsi
        popq %rdi
        
        ret





uthread_prepare:

        // %rdi is the first argument
        // %rdi is Context*
        // (%rdi) is allocated void* rsp
        // save allocated rsp from structure pointer
        movq (%rdi), %rax

        

        // there are 9 arguments: uthread_handle, f, data and 6 callee saved registers
        // after all popq's in uthread_switch, rsp will be at STACK_FRAME_SIZE - 16
        // after ret, rsp will be at start + STACK_FRAME_SIZE - 8
        // after push %rbp in uthread_handle, rsp will be at start + STACK_FRAME_SIZE - 16
        // start in bytes is 16-byte alligned, malloc guarantees
        // STACK_FRAME_SIZE can be divided by 16 too
        // => start + STACK_FRAME_SIZE - 16 is 16-byte alligned too therefore


        // %rsi is uthread_handle
        // start + STACK_FRAME_SIZE - 16
        movq %rsi, 64(%rax)
        // %rdx is f => it is future %rdi for uthread_handle
        // start + STACK_FRAME_SIZE - 24
        movq %rdx, 56(%rax)
        // %rcx is data => it is future %rsi for uthread_handle
        // start + STACK_FRAME_SIZE - 32
        movq %rcx, 48(%rax)


        // push callee-saved registers
        // start + STACK_FRAME_SIZE - 40
        movq %rbx, 40(%rax)
        // start + STACK_FRAME_SIZE - 48
        movq %rbp, 32(%rax)
        // start + STACK_FRAME_SIZE - 56
        movq %r12, 24(%rax)
        // start + STACK_FRAME_SIZE - 64
        movq %r13, 16(%rax)
        // start + STACK_FRAME_SIZE - 72
        movq %r14, 8(%rax)
        // start + STACK_FRAME_SIZE - 80
        movq %r15, (%rax)



        ret
