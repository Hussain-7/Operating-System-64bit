;Now the code is just initializing TSS PIT and PIC all the handler and other code will be adder to other files

section .data

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






section .text
;In text section the interrupt handlers are not defined in kernel file and so the can be implemented in other files

;Since the kernal main function is not defined here but in the c file
extern KMain
;Making a global start label so the linker will know that the start is entry of the kernel
global start

start:
   lgdt [Gdt64Ptr]
   
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
   
   ;When we enter the kernel entry we can jump to the main function in c
   ;Before we call the main function we need to point are stack pointer to the correct position
    mov rsp,0x2000000
    call KMain

End:
   hlt
   jmp End
