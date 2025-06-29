# User-Space Context Switching with ucontext

This repository demonstrates user-space context switching using the POSIX `ucontext` family of functions. These functions allow you to save and restore execution contexts, enabling cooperative multitasking and coroutine-like behavior.

## What is Context Switching?

Context switching is the process of storing the state of a process or thread so that it can be restored and resumed from the same point later. While the operating system typically handles context switching between processes/threads, the `ucontext` functions allow you to implement this at the user level within a single process.

## Code Overview

```c
#include <ucontext.h>
#include <stdio.h>

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
```

## How It Works

### 1. Context Structure Setup
- `ucontext_t ctx1, ctx2`: Two context structures to store CPU state
- `char stack1[8192]`: Custom stack for the new execution context

### 2. Context Configuration
- `getcontext(&ctx1)`: Captures current CPU state (registers, stack pointer, etc.)
- `ctx1.uc_stack.ss_sp = stack1`: Assigns custom stack
- `ctx1.uc_stack.ss_size = sizeof(stack1)`: Sets stack size
- `ctx1.uc_link = &ctx2`: Defines where to return when function completes
- `makecontext(&ctx1, f, 0)`: Modifies context to execute function `f()` with 0 arguments

### 3. Execution Flow
1. `swapcontext(&ctx2, &ctx1)`: Saves current state to `ctx2`, switches to `ctx1`
2. Execution jumps to function `f()` using the custom stack
3. `f()` prints "In f()" then calls `setcontext(&ctx2)`
4. `setcontext(&ctx2)` restores the saved state, returning to after `swapcontext`
5. Program continues and prints "Done"

## Key Functions

| Function | Purpose |
|----------|---------|
| `getcontext()` | Save current execution context |
| `setcontext()` | Restore and switch to a context |
| `makecontext()` | Modify context to call a specific function |
| `swapcontext()` | Save current context and switch to another |

## Expected Output

```
In f()
Done
```

## Compilation and Running

```bash
# Compile
gcc -o context_demo context_demo.c

# Run
./context_demo
```

**Note**: On some systems, you may need to link with `-lucontext` or define `_GNU_SOURCE`.

## Use Cases

### Coroutines
Implement cooperative multitasking where functions can yield control and resume later:

```c
// Pseudo-code for coroutine
void coroutine1() {
    printf("Coroutine 1 - Part 1\n");
    swapcontext(&ctx1, &ctx2);  // yield to coroutine2
    printf("Coroutine 1 - Part 2\n");
}
```

### State Machines
Implement complex state machines where each state has its own execution context.

### User-Space Threading
Create lightweight threading systems without kernel involvement.

### Exception Handling
Implement sophisticated error handling mechanisms with non-local jumps.

## Important Considerations

### Portability
- `ucontext` functions are POSIX but deprecated in some standards
- Some modern systems may not support them or require special compilation flags
- Consider alternatives like `setjmp`/`longjmp` for simpler use cases

### Stack Management
- Custom stacks must be properly sized to avoid overflow
- Stack memory should remain valid for the context's lifetime
- Consider stack alignment requirements on different architectures

### Signal Safety
- `ucontext` functions are not async-signal-safe
- Avoid using them in signal handlers

## Alternatives

### setjmp/longjmp
Simpler but less powerful:
```c
#include <setjmp.h>
jmp_buf buf;

if (setjmp(buf) == 0) {
    // first call
    longjmp(buf, 1);
} else {
    // returned from longjmp
}
```

### Modern Alternatives
- **C++20 Coroutines**: Language-level support for coroutines
- **Boost.Context**: Cross-platform context switching library
- **libco**: Lightweight coroutine library
- **Fiber libraries**: Various user-space threading implementations

## Security Notes

- Custom stacks bypass some stack protection mechanisms
- Ensure proper bounds checking when using custom stacks
- Be aware of potential security implications in production code

## Further Reading

- [POSIX ucontext documentation](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/ucontext.h.html)
- [Linux man pages for ucontext](https://man7.org/linux/man-pages/man3/getcontext.3.html)
- [Coroutines and cooperative multitasking](https://en.wikipedia.org/wiki/Coroutine)

## License

This example code is provided for educational purposes. Use and modify as needed for your projects.
