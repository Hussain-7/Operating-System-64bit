;Two directives 
;It tells is that bootcode is running in 16 bit mode
[BITS 16]
[ORG 0x7e00]



start:
    mov [DriveId],dl
    ;passing this value to eax will return processor feature when eax is passed as cpuid directive parameter
    ;Info about long mode support is stored in edx using test we check if bit 29 is set in edx then long mode else not supported
    ;I gb page support check on bit 26 we test it using test function as well
    mov eax,0x80000000
    cpuid
    cmp eax,0x80000001
    jb NotSupport

    mov eax,0x80000001
    cpuid
    test edx,(1<<29)
    jz NotSupport

    test edx,(1<<26)
    jz NotSupport

    mov ax,0x2000
    mov es,ax












GetMemInfoStart:
   ;in this function we get the memory map
   ;we use bios function we use system map memory function
   ;it returns memory blocks

   ;offset   field
   ; 0       base address
   ; 8       length
   ; 16      type

    mov eax,0xe820
    mov edx,0x534d4150
    mov ecx,20
    mov dword[es:0],0 ;so physical address it points to is address 0x20000 
   
    mov edi,8
    xor ebx,ebx
    ;0x15 is a bios interrupt call to obtain memory info
    int 0x15
    jc NotSupport

GetMemInfo:
    ;each memory block is 20 byte so to recieve next we add 20 bytes to edi
    ;we want to find memory region with type 1 the free region
   cmp dword[es:di+16],1
    jne Cont
    cmp dword[es:di+4],0
    jne Cont
    mov eax,[es:di]
    cmp eax,0x30000000
    ja Cont
    cmp dword[es:di+12],0
    jne Find
    add eax,[es:di+8]
    cmp eax,0x30000000 + 100*1024*1024
    jb Cont

Find:
    mov byte[LoadImage],1

Cont:
    add edi,20
    inc dword[es:0]
    test ebx,ebx
    jz GetMemDone

    mov eax,0xe820
    mov edx,0x534d4150
    mov ecx,20
    int 0x15
    jnc GetMemInfo

GetMemDone:
    cmp byte[LoadImage],1
    jne ReadError
 

TestLA20:
  mov ax,0xffff
  mov es,ax
  mov word[ds:0x7c00],0xa200  ; ds=0 so physical address =0x16 +0x7c00=0x7c00
  ;since there is high change that address 0x10700 get trucated to 0x0700 that why we compare it with 0x0700 's contents
  cmp word[es:0x7c10],0xa200  ; es=ffff so physical address = 0xffffx16 + 0x7c10=0x107c00  if content at this is not equal 0xa200 than we confirm that we have access to memory address 0x107c00
  jne SetA20LineDone
  mov word[0x7c00],0xb200
  cmp word[es:0x7c10],0xb200
  je End
 
 SetA20LineDone:
    xor ax,ax
    mov es,ax

  

SetVideoMode:
    mov ax,3
    int 0x10
    ;screen is 80 x 25  and 1 character takes 2bytes
    ;1 byte for character and 2nd byte upper half for Background and lower half for foreground color

    cli
    ;loading gdt and idt structure global and interrupt descriptor tables
    ;There is register called globalm descriptor register which points to memory
    ;where gdt stores and we load this register with address and size of gdt
    lgdt[Gdt32Ptr]

    ;After we load gdt and idt we renable protected mode by mode in enable bit in cr0
    ; cr0 is a control register with protected mode enable bit so it controls behaviour of the processor
    mov eax,cr0
    or eax,1
    mov cr0,eax
   ;now we have enable protected mode

   ;loading code segment's register is differnet from other segment registers
   ;we cannot use mov instead we use jmp


LoadFS:
    mov ax,0x10
    mov fs,ax

    mov eax,cr0
    and al,0xfe
    mov cr0,eax

BigRealMode:
    sti
    mov cx,203*16*63/100
    xor ebx,ebx
    mov edi,0x30000000
    xor ax,ax
    mov fs,ax

ReadFAT:
    push ecx
    push ebx
    push edi
    push fs
    
    mov ax,100
    call ReadSectors
    test al,al
    jnz  ReadError

    pop fs
    pop edi
    pop ebx

    mov cx,512*100/4
    mov esi,0x60000
    
CopyData:
    mov eax,[fs:esi]
    mov [fs:edi],eax

    add esi,4
    add edi,4
    loop CopyData

    pop ecx

    add ebx,100
    loop ReadFAT

ReadRemainingSectors:
    push edi
    push fs

    mov ax,(203*16*63) % 100
    call ReadSectors
    test al,al
    jnz  ReadError

    pop fs
    pop edi
    
    mov cx,(((203*16*63) % 100) * 512)/4
    mov esi,0x60000

CopyRemainingData: 
    mov eax,[fs:esi]
    mov [fs:edi],eax

    add esi,4
    add edi,4
    loop CopyRemainingData

    cli
    lidt [Idt32Ptr]

    mov eax,cr0
    or eax,1
    mov cr0,eax

    jmp 08:PMEntry

ReadSectors:
    mov si,ReadPacket
    mov word[si],0x10
    mov word[si+2],ax
    mov word[si+4],0
    mov word[si+6],0x6000
    mov dword[si+8],ebx
    mov dword[si+0xc],0
    mov dl,[DriveId]
    mov ah,0x42
    int 0x13
    
    setc al
    ret

ReadError:
NotSupport:
    mov ah,0x13
    mov al,1
    mov bx,0xa
    xor dx,dx
    mov bp,Message
    mov cx,MessageLen 
    int 0x10
End:
    hlt
    jmp End
    

[BITS 32]
PMEntry:
   mov ax,0x10
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov esp,0x7c00


    ;this block of code till lgdt initialize paging structure used to translate virtual address to physical address
    cld
    ;setting address of cr3 which is also the address of PM4 table
    ;each entry in pml4 represent 512gb but we only implement lower 1gb
    mov edi,0x70000
    xor eax,eax
    mov ecx,0x10000/4
    rep stosd
    
    ;each tables takes up 4kb space since 512 entries each of 8ytes each
    ;setting entry of pml4 table to next tables address which is after 4kb hence 1000h added
    ;and the last 3 bits are set to 011 
    mov dword[0x70000],0x71003
    ;setting entry of page directory table
    mov dword[0x71000],10000011b

    mov eax,(0xffff800000000000 >> 39)
    and eax,0x1ff
    ;Now we have the index to locate coresponding entry in the table
    mov dword[0x70000 + eax*8],0x72003
    mov dword[0x72000],10000011b

    lgdt [Gdt64Ptr]
    ;To enable 64bit mode now we have to set nesseary bit to 1

    ;bit 5 in cr4 is called physical address extension or PAE bit must be set to
    mov eax,cr4
    or eax,(1<<5)
    mov cr4,eax

    ;Copy the address of page structure we just set up to the cr3 register
    mov eax,0x70000
    mov cr3,eax

    ;the bit 8 of model specific register should be set to 1 to enable long mode
    ;FOr that We move index register to ecx
    mov ecx,0xc0000080
    rdmsr
    ;return value from rdmsr is in eax
    or eax,(1<<8)
    ;After making 8th bit in eax value 1 ,we can write it back we use write eax to model specific register function 
    wrmsr

    ;Also have to enable bit 31 in cr0 register
    ;we can do it the similar way
    mov eax,cr0
    or eax,(1<<31)
    mov cr0,eax
 
    ;Since each entry is 8 bytes and each entry register is 8 bytes and code segment descriptor is second enter and then offset long mode entry is 8
    jmp 8:LMEntry

PEnd:
    hlt
    jmp PEnd
  

[BITS 64]
LMEntry:
    ;FOr 64 bit mode we only initialize stack pointer hence we set it to 7c00
    mov rsp,0x7c00
    ;clearing direction flag so that move instruction will process data from low memory address to high memory address or in short data is copied in foward direction
    cld
    ;the Destination address is in rdi register and Source address is in rsi register
     mov rdi,0x100000
    mov rsi,CModule
    mov rcx,512*15/8
    rep movsq

    ;since the kernel is reallocated to new virtual adddress which is far away from the load hence we save it here
    mov rax,0xffff800000100000   
    jmp rax

LEnd:
    hlt
    jmp LEnd
    


Message:    db "We have an error in boot process"
MessageLen: equ $-Message

ReadPacket: times 16 db 0
DriveId: db 0
LoadImage: db 0

;gdt 32 initializtion in protected
Gdt32:
   ;Since first entry in global descriptor table is empty and since each entry in gdt is 8bytes we set it to 0 using dq   dq 0
   dq 0
Code32:
    ;since first 2 bytes of code segment are first two bytes which we want to set to max
    ;Next 24 bytes are the base address hence we set it to zero
    ;Next 
   dw 0xffff
   dw 0
   db 0
   ;                      7 6  4 3    0 bit
   ;Next byte 1 byte is  [P|DPL|S|TYPE]
                         ;1 00  1 1010=0x9a hex
   ;S Field is 1 bit mean segment descriptor is system descriptor or not we set it to 1 meaning it is data segment descriptor
   ;Type field assigned to 1010 this field is that whether code segment is confirming or not confirming 
   ;Dpl indicates priveledge level of segment hence we set it to 0
   ;p is present bit so we set it ti 1 when we load descriptor otherwise cpu exception is generated

    db 0x9a

    ;Next byte is [G|D|0|A|LIMIT] it is combination of segment size and attributes
    ;               1 1 0  0 1111 =0xcf
    ;lower 4 bit limits is size so we set it to max
    ;A - Available bit can be used by system software so we siply ignore it
    ;D bit is one then it mean default operand size is 32bit other wise operand size is 16bit so we set it 32bit in protected mode
    ;G is granurality bit which mean size is scaled BY 4kb which gives us max of 4gb
    
    db 0xcf
    db 0
   
     
Data32:
    ;Data segement initialization is sane as code except a few changes
    ;type field in this byte is changed [P|DPL|S|TYPE]
    ;it is set to 0010 which mean data segement is readable and writable data segment and hence [P|DPL|S|TYPE] =0x92 hex
   dw 0xffff
   dw 0
   db 0
   db 0x92
   db 0xcf
   db 0

;The code and data segment descritpors is all we need in protected mode

Gdt32Len: equ $-Gdt32

Gdt32Ptr: dw Gdt32Len-1
          dd Gdt32

Idt32Ptr: dw 0
          dd 0


Gdt64:
    ;first segment empty hence set to 0
    dq 0
    ;left over [D L P DPL 1 1 C]
    ;           0 1 1 00      0
    dq 0x0020980000000000
    

Gdt64Len: equ $-Gdt64


Gdt64Ptr: dw Gdt64Len-1
          dd Gdt64

CModule: