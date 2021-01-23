;Two directives 
;It tells is that bootcode is running in 16 bit mode
[BITS 16]
[ORG 0x7c00]

;These are labels
start:
    xor ax,ax   
    mov ds,ax
    mov es,ax  
    mov ss,ax
    mov sp,0x7c00



TestDiskExtension:
    mov [DriveId],dl
    mov ah,0x41
    mov bx,0x55aa
    int 0x13
    jc NotSupport
    cmp bx,0xaa55
    jne NotSupport


LoadLoader:
    ;ReadPacket : which mean we want to use disk extension service
    ; Offset   field
    ;   0      size of reapacket
    ;   2      number of sectors and sector size is assumed to be 512bytes
    ;   4      offset
    ;   6      segment
    ;   8      address lo
    ;   12     address hi
    ; si is the index register 
    ;Boot file consumes 1 sector and loader consume next 5 sectors and hence we assign 6th sector to Kernel


    mov si,ReadPacket
    mov word[si],0x10
    mov word[si+2],15
    mov word[si+4],0x7e00
    mov word[si+6],0
    mov dword[si+8],1
    mov dword[si+0xc],0
    mov dl,[DriveId]
    mov ah,0x42;means we want to use disk extension service
    int 0x13
    jc  ReadError

    mov dl,[DriveId]
    jmp 0x7e00 

ReadError:
NotSupport:
    
    ;Function paramater value as stored as follwing
    ;ah : hold function code so used 0x13 mean print screen
    ;al : specifies write mode so we set it to 1 so cursor will be placed at end of screen bl storing 00000101 or a mean green color character
    ;bx :  bh holds page number bl holds information about character attribute
    ;dx : dh represent row and dl represent columns we set it to zero since we want to start at beginning of the screen
    ;bp : holds address of string we want to print
    ;cx : hold len of string we want to print
    
    mov ah,0x13
    mov al,1
    mov bx,0xa
    xor dx,dx
    mov bp,Message
    mov cx,MessageLen 
    int 0x10
End:
    
   ;hlt palces processor in halt state so that interrupts can execute
   ;jump End for infine loop to keep on checking interrups
    
    hlt    
    jmp End



;db mean data byte used to declare string
;$ gives current stack position - address of message which give length of messae

DriveId:    db 0
Message:    db "We have an error in boot process"
MessageLen: equ $-Message
ReadPacket: times 16 db 0
;$$ mean start of code $ means end of message and address is below the end of message address in stack as stack grow downwards

times (0x1be-($-$$)) db 0

    ;These walue can be checked using command hexdump -C os.img
    db 80h          ;Boot indicator
    db 1,1,0        ;staring CHS   C-cylinder h-height  s-sector
    db 06h         ;type
    db 0fh,03fh,0cah  ;ending CHS
    dd 3fh             ;starting sector
    dd 031f11h  ;size	
    times (16*3) db 0

    ;sector size 512 bytes
    db 0x55
    db 0xaa

	