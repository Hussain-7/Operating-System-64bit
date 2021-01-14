section .text
global writeu 

writeu:
    ;to allocate 16 byte space on stack for the arguments we subtract 16 from rsp which points to stack top
    sub rsp,16
    ;since rax hold index for system call function we zero it
    ;since index of write screen function is zero
    xor eax,eax

    ;first argument is in rdi and second in rsi which we save in the new allocated space
    mov [rsp],rdi
    mov [rsp+8],rsi

    ;now for interrupt rdi hold number of arguments which are 2
    ;rsi holds address of arguments
    mov rdi,2
    mov rsi,rsp
    int 0x80

    add rsp,16
    ret
