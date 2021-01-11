section .text
global memset
global memcpy
global memmove
global memcmp

;cld - clear direcion flagso it copies the data from low memory address to high memory address

memset:
    cld
    ;memset params  1st          2nd         3rd         
    ;               rdi (buffer) rsi (value) rdx (size)  
    mov ecx,edx
    mov al,sil
    ;we store the value al at the memory referenced by rdi using stosb
    rep stosb
    ret

memcmp:
    cld
    ;memcmp params  1st          2nd         3rd        4th 
    ;               rdi (dst addr) rsi (src addr) rdx (size) 
    xor eax,eax
    mov ecx,edx
    ;compare value in rdi to rsi and will sett flags accordingly.
    ;using rep until they are equal or ecx is non zero this process will be repeated 
    repe cmpsb
    ;if zero flag is set ,setting al to 1
    setnz al
    ret

memcpy:
memmove:
    ;so there are two scenario in case of copy function
    ;Case 1:  In which start of source area is before the destination and the end is in destination
    ;in this case we have to copy data backwards otherwise data at the back of the source will be replaced by data in the front
    ;Case 2:  not overlap 
    ;so we can simply copy the data
    cld
    ;memcpy params  1st          2nd         3rd        4th 
    ;               rdi (dst addr) rsi (src addr) rdx (size)  
    cmp rsi,rdi
    jae .copy
    mov r8,rsi
    ;using this we get the address of end of src string and we can copy in reverse order 
    add r8,rdx 
    cmp r8,rdi
    ;if r8 <= rdi we jump to copy part
    jbe .copy

.overlap:
    ;set direction flag to copy data from hogh memory address to low memory address
    std
    add rdi,rdx
    add rsi,rdx
    sub rdi,1
    sub rsi,1

.copy:
    mov ecx,edx
    rep movsb
    cld
    ret
    