#include <ucontext.h>

ucontext_t ctx1, ctx2;
char stack1[8192];

void f() {
    printf("In f()\n");
    setcontext(&ctx2); // switch back
}

int main() {
    getcontext(&ctx1);
    ctx1.uc_stack.ss_sp = stack1;
    ctx1.uc_stack.ss_size = sizeof(stack1);
    ctx1.uc_link = &ctx2;
    makecontext(&ctx1, f, 0);

    swapcontext(&ctx2, &ctx1);  // switch to ctx1
    printf("Done\n");
}
