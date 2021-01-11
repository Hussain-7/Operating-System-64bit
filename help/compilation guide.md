
## Compilation of c file ##
Command: gcc -std=c99 -mcmodel=large -ffreestanding -fno-stack-protector -mno-red-zone -c main.c

-std=c99 -we use c99 standard 
-mcmodel=large - we use large model so that code generated is for large mode otherwose we could have rlocation truncated to fit error
-ffreestanding - which tell gcc we dont need c standard library and other run time features .In freestanding environment there are still some header available to be used such as stdint we use
-fno-stack-protector is used to enable it we need run time support which we donot enable so here we should disable this feature hence we use this
-mno-red-zone - it is actually a area of 128 bytes below stack pointer which can be used by leaf function wothout changinf rsp register.Red zone is specified in system vAMD64 calling convention which we use in the code
so we need to disable redzone in the kernel otherwise kernel stack could be corrupted if the interrup occurs.
-c - means we only want to compile the file into object file without the linking to exe file
at the end we write name of file to compile


## linker Command ##
-> We have two object file kernel.o and main.o and to link them and generate kernel we use the following command

Command : ld -nostdlib -T linker.lds -o kernel kernel.o main.o  


-nostdlib - which means we donot use startup files and library files
-o - after this we specify the name of file we want to produce 
kernel - is the name of file we want to produce
kernel.o and main.o - are the name of object file we want to link
-T - we specify name of linker file after this

-> when we generate kernel it is an elf format file and what we want is to generate a binary file so that we can load it to memory
and jump from the loader as we did before

Command: objcopy -o binary kernel kernel.bin

-o : after -o we specify the format to which we what to convert the file and then the name of file we want to convert.At last the name of file produced kernel.bin


