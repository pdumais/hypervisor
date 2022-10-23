// This is stage #2. This is where we have fun and do stuff

.global setup_lapic
.global setup_idt
.global load_idt

.global outport
.global inport
.global halt
.global hypercall
.extern startAllCPUs
.extern main

.globl _start
.code64
.align 16
_start:
    mov     $0xFFFE,%rdx // get cpu index
    xor     %rax,%rax
    in      %dx,%eax
    cmp     $0,%rax
    jnz     1f
    call    setup_idt
1:
    call    load_idt
    call    setup_lapic

    // As a proof of concept, we'll just leave all APs spinning here
    mov     $0xFFFE,%rdx // get cpu index
    xor     %rax,%rax
    in      %dx,%eax
    1:
    mov     $0xFFFF,%rdx
    out     %eax, %dx
    cmp     $0,%rax
    jnz 1b

    call    startAllCPUs

///////////////////////////////////////////////////////
//1:
//    mov     $0xFFFF,%dx
//    out     %eax, %dx
//    jmp 1b
///////////////////////////////////////////////////////




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