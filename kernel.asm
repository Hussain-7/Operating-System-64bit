[BITS 64]
[ORG 0X200000]




start:

   mov rdi,Idt

   ;Interrupt Handler 1
   mov rax,Handler0
   ;now rdi holds address of Idt and rax hold address of offset of handler 0
   ;An we know that offset is divided into 3 parts(total of 64 bit)lower 16bit is in the first two bytes
   mov [rdi],ax
   shr rax,16
   ;now second part of offset is in ax
   ;since next offset bit location is at 7th byte we add 6 to rdi so it points to 7th byte
   mov[rdi+6],ax
   ;now third part of offset 32bits is in eax
   ;since next offset bit location is at 9th byte we add 8 to rdi so it points to 9th byte
   shr rax,16
   mov[rdi+8],eax

   ;Interrupt Handler 2
   mov rax,Timer
   ;rax hold address of offset of Timer and then we follow same steps as for handler
   ;rdi holds the address of idt and so we add rdi 32*16 so it points to timer entry
   ;vector numnber for timer is set to 32 in the PIC so address of entry is base of idt
   ;so  address of entry must be base of idt plus 32*16 ,keeping in mind each entry takes 16 byte space
   add rdi,32*16
   mov [rdi],ax
   shr rax,16
   mov[rdi+6],ax
   shr rax,16
   mov[rdi+8],eax

   lgdt [Gdt64Ptr]
   lidt [IdtPtr]
   

   
   ;To get code segment descriptor instead of using jump here we will use a new type of instruction
   ;We push the operand needed to load code segment to stack
   push 8  ;code segment selector
   push  KernelEntry ;Offset - so here we give the address of location we want to branch in
   db 0x48
   ;Default operand size of for return is 32bit hrnce we also add operand size override param
   retf

KernelEntry:
   mov byte[0xb8000],'K'
   mov byte[0xb8001],0xa
   
   ;Used to check div by zero interrupt
   ;xor rbx,rbx
   ;div rbx

InitPIT:
;There are 3 channels in the PIT,through channel 0 to channel 2
;channel 1 and 2 are not used in our system so we only use channel 0
;PIT has 4 registers,1 mode command register and other 3 data registers
;Mode command register has 4 parts in it.
;bit 0 is 0 which mean binary form else bcd for if 1
;bit 1-3 is operating mode we set it to 010 which is mode 2 used for recorring interrup
;bit 4 and 5 are access mode.There are 3 data registers of 8 bit each if we want to write two bits in a row and hence access mode specifies in which order data will be written low byte first or high byte first
;we use 11 for 4 , 5 bit which means low byte first
;last two are for selecting channel and we select channel 0
;7 6 5 4 3 2 1 0 bit 
;0 0 1 1 0 1 0 0 = 0x34
    
  mov al,(1<<2)| (3<<4)
  ;address of mode instruction is 0x43
  out 0x43,al
 ;Now writing in data register
 ;we want to write interrupt interval value in data registers
 ;pit work such that it will decrement the value provided roughly 1.2 milliontimes/s
 ;so if we want 100 times per second so rougly 1.2 mil/100 =12000 
  mov ax,11931
  ;address of data register of channel 0 is 40
  out 0x40,al 
  mov al,ah
  out 0x40,al
  ;PIT is now configured



InitPIC:
   ;Same as the PIT chip PIC also has command and data registers
   ;the address for command register of master chip is 20 and slave is a0
   ;initialization command word 1 is 7 6 5 4 3 2 1 0 bit
   ;                                 0 0 0 1 0 0 0 1 = 0x11
   ;bit 4 means this is initialization command followed by 3 initialization command words we are about to write
   ;bit 0 inidicats that we will use last initialization command word
   mov al,0x11
   out 0x20,al
   out 0xa0,al
   
   ;2nd initialization command word
   ;it has starting vector number for the first IRQ
   ;since processor has defined first 32 for its own use so we can define from 32 to 155
   ;so we can assign 32 to first word command
   mov al,32
   ;to store data to data register whose address is 21 and a1 for master and slave
   out 0x21,al
   ;starting vector for slave is 40 in this case so we copy 4- to slave's data register
   mov al,40
   out 0xa1,al
   ;3rd initialization of command word
   ;this word indicates that which IRQ is used for connecting two PIC chips
   ;initialization command word 3 is 7 6 5 4 3 2 1 0 bit
   ;                                  0 0 0 0 0 1 0 0= 0x4
   ;since on regular system,the slave is attached to master via IRQ 2 hence we set bit 2
   mov al,4
   out 0x21,al
   ;word for slave is identification so it should be set to 2
   mov al,2
   out 0xa1,al

   ;last command word Initialization which is used for selecting mode.
   ;bit 0 is set to 1 meaning that x86 system used
   ;bit 1 is autoatic end of interrupt we dont want to set it so let it be 0
   ;bit 2 and 3 are used to set buffered mode we dont want that hence we set them 0
   ;bit 4 specifies fully nested mode we dont use it hence 0
   ;6 and 7th bit not use hence set to 0
   ;initialization command word 3 is 7 6 5 4 3 2 1 0 bit
   ;                                 0 0 0 0 0 0 1= 0x1

   mov al,1
   out 0x21,al
   out 0xa1,al
   ;Now interrupt controller is working
   ;One more thing to do since we have total 15 Irqs in PIC,we only set up one device  the Pit 
   ;hence we mask all Irqs except irq0 which is used by the master which pit uses
   ;Setting all irqs of master except 1st to zero
   mov al,11111110b
   out 0x21,al
   ;setting all slave irqs to zero since we donot use it at the moment
   mov al,11111111b
   out 0xa1,al

   ;Now only Irq0 of master will fire interrupts
   
   ;As when we switched from realmode to protected mode in loader file we disabled the interrupt
   ;so we need to enable the interrupts again
   
   ;set interrupt flag function
   sti


End:
   hlt
   jmp End



Handler0:
    push rax
    push rbx  
    push rcx
    push rdx  	  
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov byte[0xb8000],'D'
    mov byte[0xb8001],0xc

    jmp End

    pop	r15
    pop	r14
    pop	r13
    pop	r12
    pop	r11
    pop	r10
    pop	r9
    pop	r8
    pop	rbp
    pop	rdi
    pop	rsi  
    pop	rdx
    pop	rcx
    pop	rbx
    pop	rax

    iretq
Timer:

    push rax
    push rbx  
    push rcx
    push rdx  	  
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov byte[0xb8010],'T'
    mov byte[0xb8011],0xe

    jmp End

    pop	r15
    pop	r14
    pop	r13
    pop	r12
    pop	r11
    pop	r10
    pop	r9
    pop	r8
    pop	rbp
    pop	rdi
    pop	rsi  
    pop	rdx
    pop	rcx
    pop	rbx
    pop	rax
    iretq

Gdt64:
    ;first segment empty hence set to 0
    dq 0
    ;left over [D L P DPL 1 1 C]
    ;           0 1 1 00      0
    dq 0x0020980000000000

Gdt64Len: equ $-Gdt64


Gdt64Ptr: dw Gdt64Len-1
          dq Gdt64

Idt:
   %rep 256
      dw  0
      dw  0x8
      db  0
      ;6th byte is Attribute byte  P DPL TYPE
      ;                1 00  01110
      db  0x8e
      dw  0
      dd  0
      dd  0
   %endrep

IdtLen: equ $-Idt

IdtPtr: dw IdtLen-1
        dq Idt