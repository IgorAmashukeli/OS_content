        .global sum
        .global sum_struct
        .global sum_array
        .global sum_va_arg

        // sum должна складывать rsi и rdi, а результат возвращать в rax
sum:
        // push old %rbp pointer on to the stack
        pushq %rbp
        // now rbp points at the start of this function code frame
        // the values before rbp are arguments
        // the values after rbp are local variables
        movq %rsp, %rbp


        // add %rsi and %rdi to %rax to get the value %rsi + %rdi
        movq %rsi, %rax
        addq %rdi, %rax


        // now stack shrinked, all the local variables left before
        // the only think to remove is %rbp
        movq %rbp, %rsp
        // remove %rbp from the stack
        popq %rbp

        // return %rax
        ret







        // sum_struct принимает указатель на структуру в регистре rdi
        //
        // структура имеет layout
        // struct {
        //     int64_t a;
        //     int64_t b;
        //     int64_t c;
        // };
        //
        // функция должна записывать в поле c значение a + b
sum_struct:     
          // push old %rbp pointer on to the stack
        pushq %rbp
        // now rbp points at the start of this function code frame
        // the values before rbp are arguments
        // the values after are local variables
        movq %rsp, %rbp

        // save a in %rbx
        movq (%rdi), %rbx
        // add b to a in %rcx
        addq 8(%rdi), %rbx
        // save a + b in c
        movq %rbx, 16(%rdi)


        // now stack shrinked, all the local variables left before
        // the only think to remove is %rbp
        movq %rbp, %rsp
        // remove %rbp from the stack
        popq %rbp

        // return
        ret






sum_array:
        // sum_array принимает указатель на массив 64битных целых
	    // чисел в регистре rdi и количество элементов в массиве
        // в регистре rsi
        //
        // функция должна возвращать сумму элементов массива в регистре rax
        // push old %rbp pointer on to the stack
        pushq %rbp
        // now rbp points at the start of this function code frame
        // the values before rbp are arguments
        // the values after are local variables
        movq %rsp, %rbp


        // counter
        movq $0, %rcx

        // number
        movq $0, %rax

loop:

        // were all numbers processed?
        cmpq %rcx, %rsi

        // if so, go to the end
        je end_arr_sum

        // save address of arr[i]
        // arr + i = %rdi + 8 * %rcx
        leaq (%rdi, %rcx, 8), %rdx

        // add up to the value
        // %rax = arr[i]
        addq (%rdx), %rax

        // increment counter
        addq $1, %rcx

        // go back to the loop
        jmp loop


end_arr_sum:

        // now stack is shrinked, all the local variables left before
        // the only think to remove is %rbp
        movq %rbp, %rsp
        // remove %rbp from the stacl
        popq %rbp

        // go back to the loop
        ret






sum_va_arg:
        // sum_va_arg принимает количество 64битных целых чисел в регистре rdi
        //
        // сами элементы лежат на стеке сразу после адреса возврата из функции

        // push old %rbp pointer on to the stack
        pushq %rbp
        // now rbp points at the start of this function code frame
        // the values before rbp are arguments
        // the values after are local variables
        movq %rsp, %rbp


        // counter
        movq $0, %rcx


        // number
        movq $0, %rax

loop_label:


        // were all numbers processed?
        cmpq %rcx, %rdi


        // if so, go to the end
        je end_arr_sum


        // save address of next value
        // %rbp points at the start of the code frame
        // before that we have: old %rbp on the stack
        // before that we have: return address
        // => offset is +8 +8 = +16
        // i-th argument is %rbp + 16 + 8 * i = %rbp + 16 + 8 * %rcx
        leaq 16(%rbp, %rcx, 8), %rdx


        // add up value
        // %rax += i-th argument
        addq (%rdx), %rax

        // increment counter
        addq $1, %rcx

        // go back to the loop_label
        jmp loop_label


end_va_arg:

        movq %rbp, %rsp
        // remove %rbp from the stack
        popq %rbp

        // return
        ret

