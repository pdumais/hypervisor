// This is stage #2. This is where we have fun and do stuff

.global setup_lapic
.global setup_idt

.global outport
.global inport
.global halt
.global hypercall

.extern main

.globl _start
.code64
.align 16
_start:
    call    setup_idt
    call    setup_lapic


    sti
    call    main

hypercall: // hypercall(rdi=cmd)
    push    %rax
    push    %rdx
    mov     $0xFFFF,%rdx
    mov     %rdi, %rax
    out     %eax, %dx
    pop     %rdx
    pop     %rax
    ret

halt:
    hlt
    ret

outport:  // outport(rdi=port, rsi=data)
    push    %rax
    push    %rdx

    mov     %rdi, %rdx
    mov     %rsi, %rax
    out     %eax, %dx

    pop     %rdx
    pop     %rax
    ret

inport:  // outport(rdi=port)
    push    %rdx
    xor     %rax,%rax
    mov     %rdi, %rdx
    in      %dx, %eax
    pop     %rdx
    ret    