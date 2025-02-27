
struct context {
    void* rip;
    void* rsp;
    void* rbp;
};

// Fun fact: gcc ignores *returns_twice* attribute and pattern matches
// function name instead.
//
// https://gcc.gnu.org/ml/gcc-patches/2016-07/msg01163.html
//
__attribute__((returns_twice)) int savectx(struct context* ctx);
__attribute__((noreturn)) void jumpctx(struct context* ctx);
