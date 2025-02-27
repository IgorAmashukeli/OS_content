        .global savectx
        .global jumpctx

savectx:
       // save context in struct
       // context is rip, rsp, rbp


       // rsp[0] contains return address
       movq 0(%rsp), %rax
       // rdi[0] is context->rip
       movq %rax, 0(%rdi)


       // now stack contains return address
       // when jumpctx is called, it should be the same, but without return address
       // it is rsp + 8
       // saved return address will be then added by jumpctx by taking rip from saved context
       leaq 8(%rsp), %rax
       // rdi[8] is context->rsp
       movq %rax, 8(%rdi)



       // there was no prologue/epilogue => %rbp in return address should be the same as it is now
       // rdi[16] is context->rbp
       movq %rbp, 16(%rdi)

       // return 0
       xorq %rax, %rax
       ret



jumpctx:

        // restore rbp
        movq 16(%rdi), %rbp

        // restore %rsp
        // now rsp is the same as it was after context save, but without return address
        movq 8(%rdi), %rsp

        // push return address of savectx on to the stack
        pushq 0(%rdi)

        // return 1
        movq $1, %rax
        ret


