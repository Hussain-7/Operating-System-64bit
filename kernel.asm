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
   
SetTss:
   mov rax,Tss
   mov [TssDesc+2],ax
   shr rax,16
   mov [TssDesc+4],al
   shr rax,8
   mov [TssDesc+7],al
   shr rax ,8
   mov [TssDesc+8],eax
   ;Now we can load selector using task register since tss descriptor is set
   
   ;Selector we use here is on address 0x20 since descripter is in fifth entry in gdt
   mov ax,0x20
   ltr ax
   ;setting up tss in done


   ;we are going so shift from ring 0 (kernel) to ring 3 (user mode)
   ;we can check in which mode we are by checking the lower 2 bits of code segment register 00 is ring 0 and so on to 11 meaning ring 3
   ;so we have prepare the code segment descriptor for ring3 and load to cs register then we can run in user mode or ring 3
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
   ;starting vector for slave is 40 in this case so we copy 40 to slave's data register
   mov al,40
   out 0xa1,al
   ;3rd initialization of command word
   ;this word indicates that which IRQ is used for connecting two PIC chips
   ;initialization command word 3 is 7 6 5 4 3 2 1 0 bit
   ;                                 0 0 0 0 0 1 0 0= 0x4
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
   ;sti
   ;comment this as dont require it now

    ;We use interrupt return to jump from ring0 to ring3
    ;what we'll have to do is prepare 5-8 bytes dara on stack
    ;Refer stack diagram for this
    ;RIP value specifies where will we return or return address simply
    ;cs selector : we will load cs register after we return
    ;R flags contain status of cpu and when we return value will be loaded to stack pointer
    ;Rsp will store the stack pointer
    ;ss Selector 
    ;Since the stack has lIFO order we push these in receverse order.
   

   ;We are using  4th ss selector hence 24bit address is 0x18 plus the dpl bit also set so plus 3 hex
    push 0x18|3 ;for ss selector: 0x18 + 0x3 hex and then push to stack 
    push 0x7c00 ;RSP:stack points should still the address we set in boot asm file at the top
    ;When we return in user mode value is loaded in rflags and interrupt is enabled
    push 0x202    ;R flags:we only set bit 1 to 1 other carry and overflow flag are set to zero and bit 9 is also set since its interrup enable bit since and after switching from ring 0 to 3 we desable interrupt hence 000....1000000010=0x202 
    push 0x10|3  ; since we want to refernece 3rd descriptor of code segment address value is 0x10 and dpl also set hence plus 0x3      
    push UserEntry
    iretq
    ;when return executes rsp sets to 0x7c00 and hence we jump to user entry
    
UserEntry:
   ;Priveledge test code commented out
   ; mov ax,cs
   ; and al,11b
   ; cmp al,3
   ; jne UEnd

   ;So to show that control is transferring  between user and timer handler constantly
   ;we increment the printed value here too
   ;when ever timer handler is called,the proesser pushes rsp and rip to stack (register stack point and register instruction pointer)
   ;and when we return from handler those register values are restored so any task we will perform can continue
   
   ;Printing to indicate that now we are in usermode
   inc byte[0xb8010]
   mov byte[0xb8011],0xF
   
   ;we now have to implement task state managment
   ;enable interrupt in user mode
   ;What we need to implement now is when we are running in user mode and a interrupt is fired then controll should be transferred from ring 3 to ring0 or kernel mode

UEnd:

   ;jmp UEnd comment this so we come out of the infinite loop and seting jmp to user entry inorder to loop through user enrty again
   jmp UserEntry


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
;At this point we recived the interrupt only once.Becasue we jump to end  right after we enter timer handler
;The timer interrupt is configured as recoccuring interrupt which is fired at every 10ms
;if we set handler to return instead of jumping to label end we can recieve interrupts again and again
;As it set up it is call every 10ms
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
    
    inc byte[0xb8020]
    mov byte[0xb8021],0xe

    ;jmp End     removed this so interrupt handler run continuously
    ;In hardware interrupts we need to acknowledge interrupts before we return from its handlers otherwise we cannot recieve interrupt again
    ;To ack int we can write value to command register PIC(Its bit 5((counting from 0) is non specfic end of interrupt) so we can set it to 1 to ack this interrupt
     mov al,0x20
     out 0x20,al ;0x20 is address of command register of master
     


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
    dq 0x0020f80000000000  ;same as first code segment descriptor except [D L P DPL 1 1 C]  dpl changed from 00 to 11 meaning priveledge level is ring 3
    ; data segment attribute are [P DPL 1 0 0 W 0] Present bit sent to 1 and 10 after dpl bits (which is indicates priveledge level or ring no) mean this is data segment the 3rd bit from left not used hencce set to 0, W bit is 1 indicating it is writable
    ;                             1 11  1 0 0 1 0  = 0xf2                 
    dq 0x0000f20000000000

;Adding this label just to clearly know here i added tss descriptor
TssDesc:
    dw TssLen-1
    dw 0 ;Not seeting here will set at runtime
    db 0
    ;Attribute value [1 DPL TYPE]
    ;                 1 00  01001
    db 0x89 
    db 0
    db 0
    dq 0
Gdt64Len: equ $-Gdt64


Gdt64Ptr: dw Gdt64Len-1
          dq Gdt64

Idt:
   %rep 256
      dw  0
      dw  0x8
      db  0
      ;6th byte is Attribute byte  P DPL TYPE
      ;                            1 00  01110
      db  0x8e
      dw  0
      dd  0
      dd  0
   %endrep

IdtLen: equ $-Idt

IdtPtr: dw IdtLen-1
        dq Idt

Tss:
   dd 0;first 4 bytes set to zero
   ;here load rsp0 and rsp1 with 0x15000 when interruot handler called
   dq 0x15000
   ;using directive time to set all other bytes to 0 12 other field all of 4 bytes each hence 88 bytes in total
   times 88 db 0
   dd TssLen



TssLen: equ $-Tss

;Tss Details
;Value of Rs0 is stored in Tss
;when control is transferred from low priveledge ring0 the value of RSP0 is loaded to RSP register
;Since we dont require ring1 and ring2 we donot set those fields
;we set Ist field also zero since we have seen previously in IRT table setting IST field in interrupt descriptor,then its the index of ist here
;for example if ist1 is set then value of ist1 must be loaded instead of rsp0 value to the rsp register
;last io permission field is also not set since it is not required

;Tss descriptor is also stored in Gdt
;In attribue setting 010010 means that this is a 64bit tss descriptor
;P and dpl are the same as in other descriptors
;G and avl bits not used here hence set to 0

;we also need a selector to reference the descriptor.but in case of tss loading selector is different
;In this case have to load selector to tss register then use selector to locate descriptor in GDT