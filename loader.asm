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

LoadKernel:
    ;ReadPacket : which mean we want to use disk extension service
    ; Offset   field
    ;   0      size of read packet fixed
    ;   2      number of sectors and sector size assumed to be 500 bytes
    ;   4      offset
    ;   6      segment
    ;   8      address lo
    ;   12     address hi
    ; si is the index register 
    
    mov si,ReadPacket
    mov word[si],0x10
    mov word[si+2],100
    mov word[si+4],0
    mov word[si+6],0x1000
    mov dword[si+8],6
    mov dword[si+0xc],0
    mov dl,[DriveId]
    mov ah,0x42
    int 0x13
    jc  ReadError

    mov ah,0x13
    mov al,1
    mov bx,0xa
    xor dx,dx
    mov bp,Message
    mov cx,MessageLen 
    int 0x10

ReadError:
NotSupport:
End:
    hlt
    jmp End

DriveId:    db 0
Message:    db "kernel is loaded"
MessageLen: equ $-Message
ReadPacket: times 16 db 0