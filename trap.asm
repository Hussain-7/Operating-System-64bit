section .text
extern handler
global vector0
global vector1
global vector2
global vector3
global vector4
global vector5
global vector6
global vector7
global vector8
global vector10
global vector11
global vector12
global vector13
global vector14
global vector16
global vector17
global vector18
global vector19
global vector32
global vector39
global sysint
global eoi
global read_isr
global load_idt
global load_cr3
global pstart
global read_cr2
global swap
global TrapReturn

Trap:
    push rax
    push rbx  
    push rcx
    push rdx  	  
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; inc byte[0xb8010]
    ; mov byte[0xb8011],0xe

    mov rdi,rsp
    call handler

TrapReturn:
    pop	r15
    pop	r14
    pop	r13
    pop	r12
    pop	r11
    pop	r10
    pop	r9
    pop	r8
    pop	rbp
    pop	rdi
    pop	rsi  
    pop	rdx
    pop	rcx
    pop	rbx
    pop	rax   
    ;to make rsp point to the original location before the after two 8bit value were pushed on stack
    add rsp,16
    iretq



vector0:
    ;first value is the error code 
    push 0
    ;second value is the index value so we know which exception is called
    push 0
    ;we are pushing this data on stack so that we can handle all the interrupts or exception in same functiion in c file
    jmp Trap

vector1:
    push 0
    push 1
    jmp Trap

vector2:
    push 0
    push 2
    jmp Trap

vector3:
    push 0
    push 3	
    jmp Trap 

vector4:
    push 0
    push 4	
    jmp Trap   

vector5:
    push 0
    push 5
    jmp Trap    

vector6:
    push 0
    push 6	
    jmp Trap      

vector7:
    push 0
    push 7	
    jmp Trap  

vector8:
    ;its error code is pushed to stack by cpu automatically
    push 8
    jmp Trap  

;vector 9 already reserved so we donot push anything for that vector       

vector10:
    push 10	
    jmp Trap 
vector11:
    push 11	
    jmp Trap
    
vector12:
    push 12	
    jmp Trap          
          
vector13:
    push 13	
    jmp Trap
    
vector14:
    push 14	
    jmp Trap 

vector16:
    push 0
    push 16	
    jmp Trap          
          
vector17:
    push 17	
    jmp Trap                         
                                                          
vector18:
    push 0
    push 18	
    jmp Trap 
                   
vector19:
    push 0
    push 19	
    jmp Trap


;hardware Interrupts

vector32:
    push 0
    push 32
    jmp Trap

vector39:
    push 0
    push 39
    jmp Trap

sysint:
    push 0
    push 0x80
    jmp Trap

eoi:
    mov al,0x20
    out 0x20,al
    ret

read_isr:
    mov al,11
    out 0x20,al
    in al,0x20
    ret

load_idt:
    lidt [rdi]
    ret
load_cr3:
    mov rax,rdi    ;in this function we just load the address to the cr3 register
    mov cr3,rax
    ret
read_cr2:
    mov rax,cr2
    ret
pstart:
    mov rsp,rdi
    jmp TrapReturn




;---------[ Working of swap function ]
;suppose the current process is process 1 and we want to switch process by calling function swap.
;The function saves the return address as well as the general-purpose registers on the stack.
;Then it saves the rsp value in the process structure.
;Next it will copy the rsp value saved in the process 2 to register rsp
;Rsp points to the registers and return address saved on the stack the last time it runs which is process 2
;Then it restores the previous state of process 2 by popping the values to the registers and return to where it was called
;So when we enter swap function, we are in the process 1, after we return from swap, we are in the process 2

swap:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ;This way we just change the kernel stack pointer and point it to a different process
    ;rdi (the address of context field in the process)
    ;rsi (the context value in the next process)
    mov [rdi],rsp
    mov rsp,rsi
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    ret 
