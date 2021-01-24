#include "syscall.h"
#include "print.h"
#include "process.h"
#include "keyboard.h"
#include "memory.h"
#include "debug.h"
#include "stddef.h"
#include "file.h"

static SYSTEMCALL system_calls[20];

static int sys_write(int64_t *argptr)
{    
    
    write_screen((char*)argptr[0], (int)argptr[1], (char)(argptr[2]));  
    return (int)argptr[1];
}

static int sys_sleep(int64_t* argptr)
{
    uint64_t old_ticks; 
    uint64_t ticks;
    uint64_t sleep_ticks = argptr[0];

    ticks = get_ticks();
    old_ticks = ticks;

    while (ticks - old_ticks < sleep_ticks) {
        //we pass -1 here.since we just want to wait for ticks
       sleep(-1);
       ticks = get_ticks();
    }

    return 0;
}


static int sys_exit(int64_t *argptr)
{
    exit();
    return 0;
}

static int sys_wait(int64_t *argptr)
{
    wait(argptr[0]);
    return 0;
}
static int sys_keyboard_read(int64_t *argptr)
{
    return read_key_buffer();
}
static int sys_get_total_memory(int64_t *argptr)
{
    return get_total_memory();
}

static int sys_get_free_memory(int64_t *argptr)
{
    return get_free_memory();
}
static int sys_get_used_memory(int64_t *argptr)
{
    return get_used_memory();
}
static int sys_clear_screen(int64_t *argptr)
{
    clear_screen();
    return 1;
}
static int sys_open_file(int64_t *argptr)
{
    struct ProcessControl *pc = get_pc();
    return open_file(pc->current_process, (char*)argptr[0]);
}

static int sys_read_file(int64_t *argptr)
{
    struct ProcessControl *pc = get_pc();
    return read_file(pc->current_process, argptr[0], (void*)argptr[1], argptr[2]);
}

static int sys_close_file(int64_t *argptr)
{
    struct ProcessControl *pc = get_pc();
    close_file(pc->current_process, argptr[0]);

    return 0;
}

static int sys_get_file_size(int64_t *argptr)
{
    struct ProcessControl *pc = get_pc();  
    return get_file_size(pc->current_process, argptr[0]);
}

static int sys_fork(int64_t *argptr)
{
     
    return fork();
}


static int sys_exec(int64_t *argptr)
{
    struct ProcessControl *pc = get_pc();
    struct Process *process = pc->current_process; 

    return exec(process, (char*)argptr[0]);
}


void init_system_call(void)
{
    system_calls[0]  = sys_write;
    system_calls[1]  = sys_sleep;
    system_calls[2]  = sys_exit;
    system_calls[3]  = sys_wait;
    system_calls[4]  = sys_keyboard_read;
    system_calls[5]  = sys_get_total_memory;
    system_calls[6]  = sys_get_free_memory;
    system_calls[7]  = sys_get_used_memory;
    system_calls[8]  = sys_clear_screen;
    system_calls[9]  = sys_open_file;
    system_calls[10] = sys_read_file;  
    system_calls[11] = sys_get_file_size;
    system_calls[12] = sys_close_file; 
    system_calls[13] = sys_fork; 
    system_calls[14] = sys_exec; 

}

void system_call(struct TrapFrame *tf)
{
    int64_t i= tf->rax;
    int64_t param_count = tf->rdi;
    int64_t *argptr = (int64_t*)tf->rsi;

    if (param_count < 0 || i> 14 || i< 0) { 
        tf->rax = -1;
        return;
    }
    
    ASSERT(system_calls[i] != NULL);
    tf->rax = system_calls[i](argptr);
}