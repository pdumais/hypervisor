/*
 * This code is mostly reused code from my OS project at https://github.com/pdumais/OperatingSystem
 */
.global setup_lapic
.global setup_idt
.global ioAPICRedirect
.global registerInterruptGate
.global registerIRQ


#define APIC_BASE_MSR                       0x1B
#define APIC_REG_ID                         0x0020
#define APIC_REG_VERSION                    0x0030
#define APIC_REG_TASKPRIORITY               0x0080
#define APIC_REG_ARBITRATIONPRIORITY        0x0090    
#define APIC_REG_PROCESSORPRIORITY          0x00A0
#define APIC_REG_EOI                        0x00B0
#define APIC_REG_LOGICALDESTINATION         0x00D0
#define APIC_REG_DESTINATIONFORMAT          0x00E0
#define APIC_REG_SPURIOUSINTERRUPTVECTOR    0x00F0
#define APIC_REG_ISR                        0x0100
#define APIC_REG_TMR                        0x0180
#define APIC_REG_IRR                        0x0200
#define APIC_REG_ERRORSTATUS                0x0280
#define APIC_REG_INTERRUPTCOMMANDLOW        0x0300
#define APIC_REG_INTERRUPTCOMMANDHIGH       0x0310
#define APIC_REG_LOCALVECTORTABLETIMER      0x0320
#define APIC_REG_PERFORMANCECOUNTERLVT      0x0340
#define APIC_REG_LOCALVECTORTABLELINT0      0x0350
#define APIC_REG_LOCALVECTORTABLELINT1      0x0360
#define APIC_REG_LOCALVECTORTABLEERROR      0x0370
#define APIC_REG_TIMERINITIALCOUNT          0x0380
#define APIC_REG_TIMERCURRENTCOUNT          0x0390
#define APIC_REG_TIMERDIVIDECONFIGURATION   0x03E0
#define APIC_SPURIOUS_VECTOR                0x4F
#define IDTSPACE        0x00006000

#define PUSHAEXCEPTRAX  push    %rdi; \
    push    %rbx; \
    push    %rcx; \
    push    %rdx; \
    push    %rsi; \
    push    %rbp; \
    push    %r8; \
    push    %r9; \
    push    %r10; \
    push    %r11; \
    push    %r12; \
    push    %r13; \
    push    %r14; \
    push    %r15

#define PUSHA   push    %rax; \
    push    %rdi; \
    push    %rbx; \
    push    %rcx; \
    push    %rdx; \
    push    %rsi; \
    push    %rbp; \
    push    %r8; \
    push    %r9; \
    push    %r10; \
    push    %r11; \
    push    %r12; \
    push    %r13; \
    push    %r14; \
    push    %r15

#define POPA    pop %r15; \
    pop %r14; \
    pop %r13; \
    pop %r12; \
    pop %r11; \
    pop %r10; \
    pop %r9; \
    pop %r8; \
    pop %rbp; \
    pop %rsi; \
    pop %rdx; \
    pop %rcx; \
    pop %rbx; \
    pop %rdi; \
    pop %rax

#define DECLAREIRQHANDLER(irq) intIRQ##irq:; \
    push    %rax; \
    mov     $((irq*16*8)),%rax; \
    add     $IOAPICHANDLERS,%rax; \
    jmp     baseIRQHandler; \

#define REGISTERINTFORIRQ(int,irq) mov $int,%rax; \
    mov     $intIRQ##irq,%rbx; \
    call    registerInterruptGate; \

registerIRQ: // registerIRQ(rdi=irq, rsi=handler_addr)
    push    %rax
    mov     %rdi,%rax
    shl     $7,%rax
    add     $IOAPICHANDLERS,%rax
    mov     %rsi,(%rax)
    pop     %rax
    ret

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: Spurious Int Handler
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
spuriousHandler:    
    iretq


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: ioAPICRead(rdi=ioApicID, esi=register)
// Return: eax=value
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
ioAPICRead:
    push    %rdi
    //TODO: The ioapic base address should not be hardcoded. We should look for it in memory
    mov     $0xfec00000, %rdi

    mov     %esi,(%rdi)
    mov     16(%rdi),%eax
    pop     %rdi
    ret

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: ioAPICWrite(rdi=ioApicID, esi=register,edx=value)
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
ioAPICWrite:
    push    %rdi
    //TODO: The ioapic base address should not be hardcoded. We should look for it in memory
    mov     $0xfec00000, %rdi
    mov     %esi,(%rdi)
    mov     %edx,16(%rdi)
    pop     %rdi
    ret

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: ioAPICRedirect(rdi=entry, esi=vector)
//  Sets a redirection at IOAPIC index to cpu vector defined in esi
//  WARNING: we only support the first apic in here. 
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
ioAPICRedirect:
    push    %rax
    push    %rdx
    push    %rdi

    mov     %rsi,%rdx
    mov     %rdi,%rsi
    // entry register = 0x10+index*2
    lea     0x10(,%rsi,2),%rsi

    // set the redirection entry. bits7:0 are the vector
    //unmasked, physical deliverymode, fixed delivery mode,  Deliver on cpu0
    //TODO: should provide a way to specify polarity and trigger mode
    //or      $(1<<15)|(1<<13),%rdx

    mov     $0,%rdi
    call    ioAPICWrite

    add     $1,%rsi
    shr     $32,%rdx
    call    ioAPICWrite

    pop     %rdi
    pop     %rdx
    pop     %rax
    ret

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: baseIRQHandler
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
baseIRQHandler:
    PUSHAEXCEPTRAX              // rax was saved before going here
    mov     $16,%rcx
1:  cmp     $0,(%rax)
    jz      baseIRQHandlerExit  // no more handlers, exit
    push    %rax                // There is no guarantee to handler will preserve registers
    push    %rcx
    call    (%rax)              
    pop     %rcx
    pop     %rax
    add     $8,%rax
    loop    1b
baseIRQHandlerExit:
    call    ackAPIC       //TODO: should indicate which IOAPIC

    POPA
    iretq

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: ackAPIC
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
ackAPIC:
    push    %rax
    push    %rdi
    mov     $1,%rax
    mov     apic_base,%rdi
    mov     %eax,APIC_REG_EOI(%rdi)
    pop     %rdi
    pop     %rax
    ret


.code64
.align 4
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: registerInterruptGate(rax=vector,rbx=handler_adress)
//  Int gates will disable interrupts on entry
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
registerInterruptGate:
    push    %rdi
    shl     $4,%rax                   // rax = rax*16
    leaq    IDTSPACE(%rax),%rdi    // rdi=IDTSPACE+rax*1
    movw    %bx,(%rdi)
    movw    $0x18,2(%rdi)
    mov     %rbx,4(%rdi)
    movw    $0xEE00,4(%rdi)         //DPL=3
    movl    $0,12(%rdi)
    pop     %rdi
    ret

apic_base: .quad 0

InterruptHandlers:
    .quad divideByZeroHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad int3Handler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandlerWithError
    .quad defaultHandler
    .quad defaultHandlerWithError
    .quad defaultHandlerWithError
    .quad defaultHandlerWithError
    .quad exceptionDHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandlerWithError
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandler
    .quad defaultHandlerWithError
    .quad defaultHandler

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Default exception handler that expects an error code
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
.align 16
defaultHandlerWithError:
    push        %rax
    mov         $0xFF,%rax
    jmp         crashDump

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Divide by zero 
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
.align 16
divideByZeroHandler:
    mov     $0,%rax
    mov     $0,%rbx
    div     %rbx        // this is a hack to tripple fault

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// int 3
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
.align 16
int3Handler:
    push    %rax
    push    %rdx
    mov     $0xFFFF,%rdx
    mov     $3, %rax
    out     %eax,%dx
    pop     %rdx
    pop     %rax
    iretq

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Default exception handler that does not expect an error code
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
.align 16
defaultHandler:
    // push a fake error code since crashDump will expect one on the stack
    pushq       $0  
    push        %rax
    mov         $0xFF,%rax
    jmp         crashDump

crashDump:
    1:  hlt    
        jmp 1b

setup_idt:
    push    %rax
    push    %rbx
    push    %rcx
    push    %rdi

    // 32 first slots are reserved for exception handlers
    mov     $IDTSPACE,%rdi
    mov     $32,%rcx
    mov     $InterruptHandlers,%rax
1:  //prepareExceptions
    mov     (%rax),%rbx
    movw    %bx,(%rdi)
    movw    $0x18,2(%rdi)           // ring0 selector since code resides in ring0
    mov     %rbx,4(%rdi)
    movw    $0xEF00,4(%rdi)         // Ef: Trap gate with DPL=3. Those dont clear IF when entring handler
    movl    $0,12(%rdi)
    add     $16,%rdi
    add     $8,%rax
    loop    1b

    call    loadIDT
    pop     %rdi
    pop     %rcx
    pop     %rbx
    pop     %rax
    ret

loadIDT:
    push    %rax
    mov     $IDTRegister,%rax
    lidt   (%rax)
    pop     %rax
    ret
.align 4
IDTRegister:
    .WORD 0x1000
    .QUAD IDTSPACE

.align 16
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Function: enableAPIC()
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
enableAPIC:
    push    %rdi
    push    %rax

    mov     apic_base,%rdi

    // unmask spurious and enable APIC
    mov     $(APIC_SPURIOUS_VECTOR | 0x100), %rax   // OR with enable flag
    mov     %eax,APIC_REG_SPURIOUSINTERRUPTVECTOR(%rdi)

    // Mask all vectors first
    mov     $0x10000,%rax
    mov     %eax,APIC_REG_LOCALVECTORTABLEERROR(%rdi)
    mov     %eax,APIC_REG_LOCALVECTORTABLETIMER(%rdi)
    mov     %eax,APIC_REG_PERFORMANCECOUNTERLVT(%rdi)
    mov     %eax,APIC_REG_LOCALVECTORTABLELINT0(%rdi)
    mov     %eax,APIC_REG_LOCALVECTORTABLELINT1(%rdi)
    xor     %rax,%rax
    mov     %eax,APIC_REG_TASKPRIORITY(%rdi)

    // unmask error vector
    //mov     $APIC_ERROR_VECTOR,%rax                                          
    //mov     %eax,APIC_REG_LOCALVECTORTABLEERROR(%rdi)

    pop     %rax
    pop     %rdi
    ret

exceptionDHandler:
        push    %rax
    mov     $0x0D,%rax
    jmp     crashDump

setup_lapic:
    push    %rdi
    push    %rsi
    push    %rax

    xor     %edx,%edx
    mov     $APIC_BASE_MSR,%ecx                                                             
    rdmsr             
    shr         $12,%rax
    shl         $12,%rax    
    // This doesn't work. I don't remeber why I needed it anyway
    //orl         $0xC0000000,%eax
    //wrmsr   
    mov     %rax, apic_base
    mov     %rdx, %rdi
    shl     $32, %rdi
    mov     %rax, %rdi

    //setup spurious interrupt for APIC
    mov     $APIC_SPURIOUS_VECTOR,%rax
    mov     $spuriousHandler,%rbx
    call    registerInterruptGate

    REGISTERINTFORIRQ(32,0)
    REGISTERINTFORIRQ(33,1)
    REGISTERINTFORIRQ(34,2)
    REGISTERINTFORIRQ(35,3)
    REGISTERINTFORIRQ(36,4)
    REGISTERINTFORIRQ(37,5)
    REGISTERINTFORIRQ(38,6)
    REGISTERINTFORIRQ(39,7)
    REGISTERINTFORIRQ(40,8)
    REGISTERINTFORIRQ(41,9)
    REGISTERINTFORIRQ(42,10)
    REGISTERINTFORIRQ(43,11)
    REGISTERINTFORIRQ(44,12)
    REGISTERINTFORIRQ(45,13)
    REGISTERINTFORIRQ(46,14)
    REGISTERINTFORIRQ(47,15)
    REGISTERINTFORIRQ(48,16)
    REGISTERINTFORIRQ(49,17)
    REGISTERINTFORIRQ(50,18)
    REGISTERINTFORIRQ(51,19)
    REGISTERINTFORIRQ(52,20)
    REGISTERINTFORIRQ(53,21)
    REGISTERINTFORIRQ(54,22)
    REGISTERINTFORIRQ(55,23)
    REGISTERINTFORIRQ(56,24)
    REGISTERINTFORIRQ(57,25)
    REGISTERINTFORIRQ(58,26)
    REGISTERINTFORIRQ(59,27)
    REGISTERINTFORIRQ(60,28)
    REGISTERINTFORIRQ(61,29)
    REGISTERINTFORIRQ(62,30)
    REGISTERINTFORIRQ(63,31)
    REGISTERINTFORIRQ(64,32)
    REGISTERINTFORIRQ(65,33)
    REGISTERINTFORIRQ(66,34)
    REGISTERINTFORIRQ(67,35)
    REGISTERINTFORIRQ(68,36)
    REGISTERINTFORIRQ(69,37)
    REGISTERINTFORIRQ(70,38)
    REGISTERINTFORIRQ(71,39)
    REGISTERINTFORIRQ(72,40)
    REGISTERINTFORIRQ(73,41)
    REGISTERINTFORIRQ(74,42)
    REGISTERINTFORIRQ(75,43)
    REGISTERINTFORIRQ(76,44)
    REGISTERINTFORIRQ(77,45)
    REGISTERINTFORIRQ(78,46)
    REGISTERINTFORIRQ(79,47)
    REGISTERINTFORIRQ(80,48)
    REGISTERINTFORIRQ(81,49)
    REGISTERINTFORIRQ(82,50)
    REGISTERINTFORIRQ(83,51)
    REGISTERINTFORIRQ(84,52)
    REGISTERINTFORIRQ(85,53)
    REGISTERINTFORIRQ(86,54)
    REGISTERINTFORIRQ(87,55)
    REGISTERINTFORIRQ(88,56)
    REGISTERINTFORIRQ(89,57)
    REGISTERINTFORIRQ(90,58)
    REGISTERINTFORIRQ(91,59)
    REGISTERINTFORIRQ(92,60)
    REGISTERINTFORIRQ(93,61)
    REGISTERINTFORIRQ(94,62)
    REGISTERINTFORIRQ(95,63)



    // Disable PIC
    mov     $0xFF,%al
    out     %al,$0xA1
    out     %al,$0x21

    call    enableAPIC


    pop     %rax
    pop     %rsi
    pop     %rdi
    ret
ipimsg:
    .quad 0
    .quad 0

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// IRQ handlers
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
DECLAREIRQHANDLER(0)
DECLAREIRQHANDLER(1)
DECLAREIRQHANDLER(2)
DECLAREIRQHANDLER(3)
DECLAREIRQHANDLER(4)
DECLAREIRQHANDLER(5)
DECLAREIRQHANDLER(6)
DECLAREIRQHANDLER(7)
DECLAREIRQHANDLER(8)
DECLAREIRQHANDLER(9)
DECLAREIRQHANDLER(10)
DECLAREIRQHANDLER(11)
DECLAREIRQHANDLER(12)
DECLAREIRQHANDLER(13)
DECLAREIRQHANDLER(14)
DECLAREIRQHANDLER(15)
DECLAREIRQHANDLER(16)
DECLAREIRQHANDLER(17)
DECLAREIRQHANDLER(18)
DECLAREIRQHANDLER(19)
DECLAREIRQHANDLER(20)
DECLAREIRQHANDLER(21)
DECLAREIRQHANDLER(22)
DECLAREIRQHANDLER(23)
DECLAREIRQHANDLER(24)
DECLAREIRQHANDLER(25)
DECLAREIRQHANDLER(26)
DECLAREIRQHANDLER(27)
DECLAREIRQHANDLER(28)
DECLAREIRQHANDLER(29)
DECLAREIRQHANDLER(30)
DECLAREIRQHANDLER(31)
DECLAREIRQHANDLER(32)
DECLAREIRQHANDLER(33)
DECLAREIRQHANDLER(34)
DECLAREIRQHANDLER(35)
DECLAREIRQHANDLER(36)
DECLAREIRQHANDLER(37)
DECLAREIRQHANDLER(38)
DECLAREIRQHANDLER(39)
DECLAREIRQHANDLER(40)
DECLAREIRQHANDLER(41)
DECLAREIRQHANDLER(42)
DECLAREIRQHANDLER(43)
DECLAREIRQHANDLER(44)
DECLAREIRQHANDLER(45)
DECLAREIRQHANDLER(46)
DECLAREIRQHANDLER(47)
DECLAREIRQHANDLER(48)
DECLAREIRQHANDLER(49)
DECLAREIRQHANDLER(50)
DECLAREIRQHANDLER(51)
DECLAREIRQHANDLER(52)
DECLAREIRQHANDLER(53)
DECLAREIRQHANDLER(54)
DECLAREIRQHANDLER(55)
DECLAREIRQHANDLER(56)
DECLAREIRQHANDLER(57)
DECLAREIRQHANDLER(58)
DECLAREIRQHANDLER(59)
DECLAREIRQHANDLER(60)
DECLAREIRQHANDLER(61)
DECLAREIRQHANDLER(62)
DECLAREIRQHANDLER(63)


IOAPICHANDLERS:
    .rept 64
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .endr
