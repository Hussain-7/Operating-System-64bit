section .text
global writeu 
global sleepu 
global waitu 
global exitu 
global keyboard_readu
global get_total_memoryu
global get_free_memoryu
global get_used_memoryu

writeu:
    ;to allocate 24 byte space on stack for the arguments we subtract 16 from rsp which points to stack top
    sub rsp,24
    ;since rax hold index for system call function we zero it
    ;since index of write screen function is zero
    xor eax,eax

    ;first argument is in rdi and second in rsi which we save in the new allocated space
    mov [rsp],rdi
    mov [rsp+8],rsi
    mov [rsp+16],rdx

    ;now for interrupt rdi hold number of arguments which are 2
    ;rsi holds address of arguments
    mov rdi,3
    mov rsi,rsp
    int 0x80

    add rsp,24
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

keyboard_readu:
    mov eax,4
    xor edi,edi
    int 0x80
    ret

get_total_memoryu:
    mov eax,5
    xor edi,edi
    int 0x80
    ret

get_free_memoryu:
    mov eax,6
    xor edi,edi
    int 0x80
    ret

get_used_memoryu:
    mov eax,7
    xor edi,edi
    int 0x80
    ret
