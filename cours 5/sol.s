// Makes enter_coroutine visible to the linker
.global enter_coroutine
enter_coroutine:
mov %rdi,%rsp
/* RDI contains the argument to enter_coroutine. */
pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx
pop %rbp
ret

.global switch_coroutine
switch_coroutine:
// call switch_coroutine pushes rip
push %rbp
push %rbx
push %r12
push %r13
push %r14
push %r15
mov %rsp,(%rdi)
mov %rsi,%rdi
jmp enter_coroutine

