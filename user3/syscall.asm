section .text
global writeu 
global sleepu 
global waitu 
global exitu 

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

sleepu:
    sub rsp,8  ;to allocate 8 byte space on stack for the arguments we subtract 8 from rsp which points to stack top
    mov eax,1  ;index no is 1 for sleep in the kernel
    mov [rsp],rdi ;(rdi consists of function arguments)so copying the argument to allocated space on stact
    mov rdi,1 ; for interrupt saving no of arguments
    mov rsi,rsp ;for interrupt we save address of args to rsi
    int 0x80
    add rsp ,8 ;to get back to original stack position
    ret

exitu:
    ;it passes zero arguments so no space allocated on stack 
    mov eax,2
    ;number of arguments zero
    mov rdi,0
    int 0x80
    ret

waitu:
    mov eax,3
    mov rdi,0
    int 0x80
    ret


