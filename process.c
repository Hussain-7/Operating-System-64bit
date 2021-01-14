#include "process.h"
#include "trap.h"
#include "memory.h"
#include "print.h"
#include "lib.h"
#include "debug.h"


extern struct TSS Tss; 
static struct Process process_table[NUM_PROC];
static int pid_num = 1;
static struct ProcessControl pc;

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

static void set_process_entry(struct Process *proc, uint64_t addr){
   uint64_t stack_top;

    proc->state = PROC_INIT;
    proc->pid = pid_num++;

    proc->stack = (uint64_t)kalloc();
    ASSERT(proc->stack != 0);

    memset((void*)proc->stack, 0, PAGE_SIZE);  
    //since stack grows downward we add stacksize to base address of stack
    //so we decrement address after pushing anything on stack   
    stack_top = proc->stack + STACK_SIZE;

     proc->context = stack_top - sizeof(struct TrapFrame) - 7*8;   
    *(uint64_t*)(proc->context + 6*8) = (uint64_t)TrapReturn;

    
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
    ASSERT(setup_uvm(proc->page_map, (uint64_t)P2V(addr), 5120));
    proc->state= PROC_READY;
}
static struct ProcessControl* get_pc(void)
{
    return &pc;
}

void init_process(void)
{  
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;
    uint64_t addr[2] = {0x20000, 0x30000};

    process_control = get_pc();
    list = &process_control->ready_list;

    for (int i = 0; i < 2; i++) {
        process = find_unused_process();
        set_process_entry(process, addr[i]);
        append_list_tail(list, (struct List*)process);
    }
}

void launch(void)
{
    struct ProcessControl *process_control;
    struct Process *process;

    process_control = get_pc();
    process = (struct Process*)remove_list_head(&process_control->ready_list);
    process->state = PROC_RUNNING;
    process_control->current_process = process;
    
    set_tss(process);
    switch_vm(process->page_map);
    pstart(process->tf);
}


static void switch_process(struct Process *prev, struct Process *current)
{
    set_tss(current);
    switch_vm(current->page_map);
    //first param is the addess of rsp member in the process and 2nd param is rsp value of process about to run
    swap(&prev->context, current->context);
}

static void schedule(void)
{
    struct Process *prev_proc;
    struct Process *current_proc;
    struct ProcessControl *process_control;
    struct HeadList *list;

    process_control = get_pc();
    prev_proc = process_control->current_process;
    list = &process_control->ready_list;
    ASSERT(!is_list_empty(list));
    
    current_proc = (struct Process*)remove_list_head(list);
    current_proc->state = PROC_RUNNING;   
    process_control->current_process = current_proc;

    switch_process(prev_proc, current_proc);   
}

void yield(void)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;
    
    process_control = get_pc();
    list = &process_control->ready_list;

    //checking if anyprocess is in ready list and simply return if no process in the list
    if (is_list_empty(list)) {
        return;
    }

    process = process_control->current_process;
    process->state = PROC_READY;
    append_list_tail(list, (struct List*)process);
    schedule();
}