// This is the first stage. This will init the CPU and memory. Will switch to 32bit protected mode and then to 64 bit long mode
// We simply create an identity page table for the first 512gb of memory. Nothing is mapped after that

.globl _start
.code16
.ORG 0                 
// TODO Will need to change the stack area eventually                                                              
.EQU STACK_END,0x7C00                                                             
.EQU STACKSEG, 0                                                                      
.EQU KERNEL_BASE_ADDR, KERNEL_BASE 

#define PAGE_TABLES 0x20000

_start:
    .BYTE 0xEA                                                                           
    .WORD 8
    .WORD 0x0 /* this will jump and will set CS to 0x0 */ 

.org 8
    cli
    // Setup stack
    mov     $0,%ax
    mov     %ax, %ss
    mov     $STACK_END,%sp                                                            
    mov     %sp,%bp

    // Switch to 32bit protected mode
    mov    %cs,%ax
    mov    %ax,%ds                                                                       
    mov     $GDTINFO,%eax                                                             
    lgdtl   GDTINFO
    mov     %cr0,%eax
    or      $1,%al                                                                    
    mov     %eax,%cr0   /* protected mode */                                          
    ljmp    $0x10,$0x200

.code32
.align 16
.ORG 0x200
    mov    $STACK_END,%esp                                                            
    mov    $8,  %ax
    mov    %ax, %ds
    mov    %ax, %es
    mov    %ax, %fs
    mov    %ax, %gs
    mov    %ax, %ss

    //mov $0x10000000, %esi
    //lodsw
    //out %ax, $111
    //mov $8,%ecx

    call    setupPageStructure
    call    enableLongMode
    ljmpl   $0x18,$longmode


setupPageStructure:                                                                   
    push    %edx                                                                      
    push    %eax                                                                      
    push    %ebx                                                                      
    push    %edi                                                                      
    push    %ecx                                                                      
    // create 1 PML4 entry (because it is good for 512gig)                            
    mov     $PAGE_TABLES,%edi                                                           
    mov     $1024,%ecx          /*clear the table first*/                             
    xor     %eax,%eax                                                                 
    rep     stosl                                                                     
    mov     $PAGE_TABLES,%edi                                                           
    mov     $(PAGE_TABLES+0x1000),%eax /* This address is aligned on 4kboundary. So bit0-11 are cleared*/
    or      $0b000000000111,%eax /* present, r/w, user/supervisor access, no pwt nor pcd*/
    xor     %ebx,%ebx       /* upper 32bit of PML4 entry. address=0 because below 4G. NX=0*/
    mov     %eax,(%edi)                                                               
    mov     %ebx,4(%edi)                                                              
    mov     %eax,(8*128)(%edi)   // same entry for for mirror                         
                                                                                      
    // Create the PDPT with 512 entries. Each entry describe 1 gig                    
    mov     $(PAGE_TABLES+0x1000),%edi                                                           
    mov     $0,%edx                                                                   
    mov     $512,%ecx         /* we need 4 of those entries that can address 1g each */  
createPDPTEntry:                                                                      
    mov     %edx,%eax       // edx is the 1gb page number                                                   
    mov     %edx,%ebx                                                                 
    shl     $30,%eax        // multiply by 1gb. 
    shr     $2,%ebx                                                                   
    orl     $0b000010000111,%eax /* present, r/w, user/supervisor access, no pwt nor pcd*/
    mov     %eax,(%edi)                                                               
    mov     %ebx,4(%edi)                                                              
    add     $8,%edi                                                                   
    inc     %edx                                                                      
    loop    createPDPTEntry                                                           
                                                                                      
    pop     %ecx                                                                      
    pop     %edi                                                                      
    pop     %ebx                                                                      
    pop     %eax                                                                      
    pop     %edx                                                                      
    ret                                               

enableLongMode:                                                                          
    push    %eax                                                                         
    push    %ecx                                                                         

    //mov     $0x80000001, %eax
    //cpuid
    //mov     %edx, %eax
    //out     %eax,$78

    // Disable paging
    mov     %cr0,%eax
    and     $0x7FFFFFFF,%eax
    mov     %eax,%cr0  

    // set PML4 address                                                                  
    mov     $PAGE_TABLES,%eax                                                        
    mov     %eax,%cr3                                                                    
                                                                                         
    // Enable PAE and PCID                                                               
    mov     %cr4,%eax                                                                    
    //or      $0b10100000,%eax                                                             
    or      $0b00100000,%eax                                                             
    mov     %eax,%cr4                                                                    
                                                                             
    // enable long mode                                                                  
    xor     %edx,%edx
    mov     $0xC0000080,%ecx                                                             
    rdmsr                           
    or      $(1<<8),%eax                                                         
    wrmsr                                                                                
                                                                                         
    //enable paging                                                                      
    mov     %cr0,%eax                                                                    
    or      $0x80000001,%eax                                                             
    mov     %eax,%cr0                                                                    
                                                                                         
    //mov     %cr4,%eax                                                                    
    //or      $0x20000,%eax           // THIS IS FOR PCID feature                          
    //mov     %eax,%cr4                                                                    
                                                                                         
    pop     %ecx                                                                         
    pop     %eax                                                                         
    ret                               


.code64
.align 16
longmode:
    mov    $STACK_END,%rsp
    jmp    0x100000 // Jump to stage 2
1: hlt
    jmp 1b


.align 4                                                                              
GDTINFO:                                                                              
    // GDT INFO                                                                       
    .WORD 0x20                                                                        
    .LONG . + 4  /*that will be the address of the begining of GDT table*/     

    // GDT                                                                            
    .LONG 00                                                                          
    .LONG 00                                                                          
                                                                                      
    // GDT entry 1. Data segment descriptor used during unreal mode                   
    .BYTE 0xFF                                                                        
    .BYTE 0xFF                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0b10010010                                                                  
    .BYTE 0b11001111                                                                  
    .BYTE 0x00                                                                        
                                                                                      
    // GDT entry 2. Code segment used during protected mode code execution            
    .BYTE 0xFF                                                                        
    .BYTE 0xFF                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0b10011010                                                                  
    .BYTE 0b11001111                                                                  
    .BYTE 0x00                                                                        
                                                                                      
    // GDT entry 3. 64bit Code segment used for jumping to 64bit mode.                
    // This is just used to turn on 64bit mode. Segmentation will not be used anymore after 64bit code runs.
    // We will jump into that segment and it will enable 64bit. But limit and permissions are ignored,
    // the CPU will only check for bit D and L in this case because when we will jump in this, we will
    // already be in long mode, but in compatibility sub-mode. This means that while in long mode, segments are ignored.
    // but not entiorely. Long mode will check for D and L bits when jumping in another segment and will change
    // submodes accordingly. So in long mode, segments have a different purpose: to change sub-modes
    .BYTE 0xFF                                                                        
    .BYTE 0xFF                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                        
    .BYTE 0x00                                                                                                                                                                                               
    .BYTE 0b10011010                                                                  
    .BYTE 0b10101111  // bit 6 (D) must be 0, and bit 5 (L, was reserved before) must be 1
    .BYTE 0x00                                             