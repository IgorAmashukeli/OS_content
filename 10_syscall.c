#include "syscall.h"

enum {
    // return value in range [-4095, -1] indicates error
    SYS_CALL_ERROR_SMALLEST = -4095,
    SYS_CALL_ERROR_BIGGEST = -1,

    // -1 is returned in C function, when error happens
    C_ERROR = -1,

    // system call numbers
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_PIPE = 22,
    SYS_DUP = 32,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT = 60,
    SYS_WAIT4 = 61,

    true = 1,

    // there is no libc => we will define NULL as 0 here
    NULL = 0
};


#define unreachable()\
    while(1) {};


int errno;


/**check return value from system call, set value for a C function error**/

// using int everywhere, cause ssize_t is signed long
// which is int on x86_64 linux and pid_t is also int
int set_ret(int ret) {

    // return value is in correct range for error
    if (ret >= SYS_CALL_ERROR_SMALLEST && ret <= SYS_CALL_ERROR_BIGGEST) {

        // set ret to C value for error
        ret = C_ERROR;

        errno = -ret;
    }

    return ret;
}


/**opens file from pathname with flags**/
int open(const char* pathname, int flags) {

    // sys_open syscall
    int number = SYS_OPEN;

    // mode is not mentioned => some arbitrary bytes from stack
    // should be put to the mode variable
    // let it be 0 for example
    // according to manual of open system call
    int mode = 0;

    // the return value of open syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // pathname is put in %rdi
            "D"(pathname),

            // flags are put in %rsi
            "S"(flags),

            // mode is put in %rdi
            "d"(mode)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and %r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**read from file descriptor fd in buf count elements**/
ssize_t read(int fd, void* buf, size_t count) {

    // sys_read syscall
    int number = SYS_READ;

    // the return value of read syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // fd is put in %rdi
            "D"(fd),

            // buf is put in %rsi
            "S"(buf),

            // count is put in %rdx
            "d"(count)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**write count elements from buf to file descriptor fd**/
ssize_t write(int fd, const void *buf, ssize_t count) {

    // sys_write syscall
    int number = SYS_WRITE;

    // the return value of write syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // fd is put in %rdi
            "D"(fd),

            // buf is put in %rsi
            "S"(buf),

            // count is put in %rdx
            "d"(count)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**pipe system call**/
int pipe(int pipefd[2]) {

    // sys_pipe syscall
    int number = SYS_PIPE;

    // the return value of pipe syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // pipefd is put in %rdi
            "D"(pipefd)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}



/**close file descriptor fd**/
int close(int fd) {
    // sys_close syscall
    int number = SYS_CLOSE;

    // the return value of close syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // fd is put in %rdi
            "D"(fd)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**dup system call**/
int dup(int oldfd) {
    // sys_dup syscall
    int number = SYS_DUP;

    // the return value of dup syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // oldfd is put in %rdi
            "D"(oldfd)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**exit system call with status**/
void exit(int status) {
    // sys_exit syscall
    int number = SYS_EXIT;


    // inline assembly
    // there is no need for "volatile" here, because of no Output Operands
    asm (

        // system call
            "syscall\n\t"

        // there are no Output Operands
            :


            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // status is put in %rdi
            "D"(status)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // nothing is returned from exit
    // it is guaranteed by exit syscall
    // but, however, for compiler not to show warnings we need to do something
    // to show there is no return
    // cause compiler thinks we are returning from no-return function here
    // therefore, we can use endless loop, for example
    unreachable();
}


/**fork system call **/
pid_t fork(void) {
    // sys_fork syscall
    int number = SYS_FORK;

    // the return value of dup syscall
    int ret = 0;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number)

        // no arguments in this system call

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}


/**waitpid system call**/
pid_t waitpid(pid_t pid, int *wstatus, int options) {

    // sys_wait4 syscall has number 1
    int number = SYS_WAIT4;

    // the return value of write syscall
    int ret = 0;

    // rusage struct for sys_wait4 system call
    void* ru = (void*) NULL;

    // inline assembly
    // "volatile" is needed here, because of Output Operands
    asm volatile(
        // move r10, because there are no "%r10" constraints
            "movq %[r10_content], %%r10\n\t"

            // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // pid is put in %rdi
            "D"(pid),

            // wstatus is put in %rsi
            "S"(wstatus),

            // options is put in %rdx
            "d"(options),

    // struct rusage* ru argument
    // there is no "r10" constraint
    // compiler should put this argument in some register/memory
    // then inline code moves it to "r10"
    [r10_content] "rm"(ru)

    :
    // system call can change memory
    "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
    );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}



int execve(const char *filename, char *const argv[], char *const envp[]) {

    // execve syscall has number 59
    int number = SYS_EXECVE;

    // the return value of execve syscall
    int ret = 0;


    // inline assembly
    // "volatile" is needed here because of Output Operands
    asm volatile (

        // system call
            "syscall\n\t"


        // %rax will contain return value
            : "=a"(ret)

            :
        // number of the syscall is put in "%rax"
            "a"(number),

            // filename is put in %rdi
            "D"(filename),

            // argv is put in %rsi
            "S"(argv),

            // envp is put in %rdx
            "d"(envp)

            :
        // system call can change memory
            "memory",

            // input/output operands can change flags
            "cc",

            // %rcx and r11 are clobbered in x86_64 system call
            "rcx", "r11"
            );

    // set return value and errno according to x86_64 system call conventions
    return set_ret(ret);
}













