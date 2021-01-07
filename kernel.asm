[BITS 64]
[ORG 0X200000]

start:
   lgdt [Gdt64Ptr]
   
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

   
End:
   hlt
   jmp End


Gdt64:
    ;first segment empty hence set to 0
    dq 0
    ;left over [D L P DPL 1 1 C]
    ;           0 1 1 00      0
    dq 0x0020980000000000

Gdt64Len: equ $-Gdt64


Gdt64Ptr: dw Gdt64Len-1
          dq Gdt64