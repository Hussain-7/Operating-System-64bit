#include "process.h"
#include "trap.h"
#include "memory.h"
#include "print.h"
#include "lib.h"
#include "debug.h"

extern struct TSS Tss; 
static struct Process process_table[NUM_PROC];
static int pid_num = 1;
void main(void);

static void set_tss(struct Process *proc)
{
    //here we just assign the top of the kernel stack to rsp0 in tss.so when we jump from ring3 to ring0 kernel stack is used
    Tss.rsp0 = proc->stack + STACK_SIZE;   
}

static struct Process* find_unused_process(void)
{
     struct Process *process = NULL;

    for (int i = 0; i < NUM_PROC; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            process = &process_table[i];
            break;
        }
    }

    return process;
}

static void set_process_entry(struct Process *proc)
{
   uint64_t stack_top;

    proc->state = PROC_INIT;
    proc->pid = pid_num++;

    proc->stack = (uint64_t)kalloc();
    ASSERT(proc->stack != 0);

    memset((void*)proc->stack, 0, PAGE_SIZE);  
    //since stack grows downward we add stacksize to base address of stack
    //so we decrement address after pushing anything on stack   
    stack_top = proc->stack + STACK_SIZE;

    
    /*In our system, the top of the kernel stack is set to the rsp0 in tss. Meaning that
      when the interrupt or exception handler is called,the stack used in this case is actually the kernel stack we set up in the process.
      In order to easily reference the data, we define the trap frame structure just as we did
      in the handler function in the trap.c file*/
    proc->tf = (struct TrapFrame*)(stack_top - sizeof(struct TrapFrame)); 
    proc->tf->cs = 0x10|3;     //we store 10hex which refrences the 3rd descriptor and alse set rpl to 3
    proc->tf->rip = 0x400000;    //rip value specifies where will we return which is the base address in usermode
    proc->tf->ss = 0x18|3;    //we push 18hex which mean we reference fourth descriptor and also set rpl to 3
    proc->tf->rsp = 0x400000 + PAGE_SIZE;    //rsp and cs both are on the same page stack grows downwards so it will be on the same page
    proc->tf->rflags = 0x202;
    
    //setting up process's kernel space
    proc->page_map = setup_kvm();
    //assuming everything goes right since it is first process
    ASSERT(proc->page_map != 0);
    //setting up process's user space
    ASSERT(setup_uvm(proc->page_map, (uint64_t)main, PAGE_SIZE));
}

void init_process(void)
{  
    struct Process *proc = find_unused_process();
    ASSERT(proc == &process_table[0]);

    set_process_entry(proc);
}

void launch(void)
{
    set_tss(&process_table[0]);
    switch_vm(process_table[0].page_map);
    pstart(process_table[0].tf);
}

void main(void)
{
    char *p = (char*)0xffff800000200020;
    *p = 1;
}
