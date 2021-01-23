section .text
global start
extern main
extern exitu

;To compile the main file, we need a start file which prepare the environment for main function.
;In the normal cases where we use c standard library, we don’t need to do this because it did it for us.
;But in our system, we have to do everything ourselves.
;After we get to ring3, the start is the first function being called and the stack is prepared when 
;we set the process entry in the process module. So we simply call the main function.
;jmp $ act as an infinite loop here we cannot use hlt since in user mode
start:
    call main
    call exitu
    jmp $