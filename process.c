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

static struct Process* alloc_new_process(void){
    uint64_t stack_top;
    struct Process *proc;
    
    proc = find_unused_process();

    if(proc == NULL)
    {
        return NULL;
    }
    proc->state = PROC_INIT;
    proc->pid = pid_num++;

    proc->stack = (uint64_t)kalloc();
    if(proc->stack == 0){
        return NULL;
    }

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
    if(proc->page_map == 0){
        kfree(proc->stack);
        memset(proc,0,sizeof(struct Process));
        return NULL;
    }

    return proc;
    
}
struct ProcessControl* get_pc(void)
{
    return &pc;
}
static void init_idle_process(void)
{
    struct Process *process;
    struct ProcessControl *process_control;

    process = find_unused_process();
    ASSERT(process == &process_table[0]);

    process->pid = 0;
    process->page_map = P2V(read_cr3());
    process->state = PROC_RUNNING;

    process_control = get_pc();
    process_control->current_process = process;
}

static void init_user_process(void)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;

    process_control = get_pc();
    list = &process_control->ready_list;

    process = alloc_new_process();
    ASSERT(process != NULL);

    ASSERT(setup_uvm(process->page_map, P2V(0x30000), 5120));

    process->state = PROC_READY;
    append_list_tail(list, (struct List*)process);
}
void init_process(void)
{  
    // struct ProcessControl *process_control;
    // struct Process *process;
    // struct HeadList *list;
    // uint64_t addr[4] = {0x20000, 0x30000,0x40000,0x50000};

    // process_control = get_pc();
    // list = &process_control->ready_list;

    // for (int i = 0; i < 4; i++) {
    //     process = find_unused_process();
    //     set_process_entry(process, addr[i]);
    //     append_list_tail(list, (struct List*)process);
    // }
    init_idle_process();
    init_user_process();
}

// void launch(void)
// {
//     struct ProcessControl *process_control;
//     struct Process *process;

//     process_control = get_pc();
//     process = (struct Process*)remove_list_head(&process_control->ready_list);
//     process->state = PROC_RUNNING;
//     process_control->current_process = process;
    
//     set_tss(process);
//     switch_vm(process->page_map);
//     pstart(process->tf);
// }


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
    if(is_list_empty(list)){
        ASSERT(process_control->current_process->pid != 0);
                current_proc=&process_table[0];
    }
    else
    {
        current_proc=(struct Process*)remove_list_head(list);
    }
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
    if(process->pid !=0){
        append_list_tail(list, (struct List*)process);
    }
    schedule();
}

void sleep(int wait)
{
    struct ProcessControl *process_control;
    struct Process *process;
    
    process_control = get_pc();
    process = process_control->current_process;
    process->state = PROC_SLEEP;
    process->wait = wait;

    
    append_list_tail(&process_control->wait_list, (struct List*)process);
    schedule();
}

void wake_up(int wait)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *ready_list;
    struct HeadList *wait_list;

    process_control = get_pc();
    ready_list = &process_control->ready_list;
    wait_list = &process_control->wait_list;
    process = (struct Process*)remove_list(wait_list, wait);

    while (process != NULL) {       
        process->state = PROC_READY;
        append_list_tail(ready_list, (struct List*)process);
        process = (struct Process*)remove_list(wait_list, wait);
    }
}

void exit(void)
{
    struct ProcessControl *process_control;
    struct Process* process;
    struct HeadList *list;

    process_control = get_pc();
    process = process_control->current_process;
    process->state = PROC_KILLED;

    list = &process_control->kill_list;
    append_list_tail(list, (struct List*)process);

    wake_up(1);
    schedule();
}

void wait(void)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;

    process_control = get_pc();
    list = &process_control->kill_list;

    while (1) {
        if (!is_list_empty(list)) {
            process = (struct Process*)remove_list_head(list); 
            ASSERT(process->state == PROC_KILLED);

            kfree(process->stack);
            free_vm(process->page_map);            
            memset(process, 0, sizeof(struct Process));   
        }
        else {
            sleep(1);
        }
    }
}