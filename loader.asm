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

    mov ah,0x13
    mov al,1
    mov bx,0xa
    xor dx,dx
    mov bp,Message
    mov cx,MessageLen 
    int 0x10

NotSupport:
End:
    hlt
    jmp End

DriveId:    db 0
Message:    db "long mode is supported"
MessageLen: equ $-Message