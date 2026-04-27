; To compile: nasm -f win64 cmpxchg16.asm && gcc cmpxchg16.obj -o cmpxchg16.exe

section .text
    global CompareExchange16

    ;                           RCX                 RDX                     R8                 R9
    ; CompareExchange16(ULONG64 volatile Dest, ULONG64 exchangeHIGH, ULONG64 exchangeLOW, ULONG64 Src)
CompareExchange16:
    ; RCX = receives first argument in windows architecture

    push rbx    ; saves RBX in stack, necessary in windows arc, as it's non-volatile
    push rdi    ; same thing

    mov rdi, rcx ; saves rcx (first argument), in rdi (rdi = arena_head)

    mov rbx, r8      ; RBX = new LOW
    mov rcx, rdx      ; RCX = new HIGH

    mov rax, [r9]   ; RAX = snapshot pointer
    mov rdx, [r9+8] ; RDX = snapshot sequence

    ; this is going to compare the value in rdi with RDX:RAX, if true, switch rdi with RCX:RBX
    ; if not, switch RDX:RAX with the value of rdi
    lock cmpxchg16b [rdi]
    je .success

.failure:
    mov [r9], rax
    mov [r9+8], rdx
    xor rax, rax
    jmp .done

.success:

    mov rax, 1
    jmp .done

.done:
    pop rdi
    pop rbx
    ret