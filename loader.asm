;Two directives 
;It tells is that bootcode is running in 16 bit mode
[BITS 16]
[ORG 0x7e00]

start:
    mov [DriveId],dl
    ;passing this value to eax will return processor feature when eax is passed as cpuid directive parameter
    ;Info about long mode support is stored in edx if 29 in edx than long mode else not supported
    mov eax,0x80000001
    test edx,(1<<29)
    cpuid 
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


DriveId:    db 0
Message:    db "loader starts"
MessageLen: equ $-Message