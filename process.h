#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "trap.h"
#include "lib.h"
#include "file.h"

//Essential data of process is stored in this structure
struct Process {
	struct List *next;
    int pid;
	int state;
	int wait;
	struct FileDesc *file[100];
	uint64_t context;
	uint64_t page_map;//stores address of pml4 table so when we run process we can switch to vm
	uint64_t stack;//Stack is used for kernel code.A process has two stacks one for user mode and other for kernel mode
	struct TrapFrame *tf;
};


//structure tss is used only for setting up stack pointer for ring0.
//We also add attribute packed so that the items in the structure are stored without padding in it.
struct TSS {
    uint32_t res0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
	uint64_t res1;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t res2;
	uint16_t res3;
	uint16_t iopb;
} __attribute__((packed));

struct ProcessControl {
	struct Process *current_process;
	struct HeadList ready_list;
	struct HeadList wait_list;
	struct HeadList kill_list;
};

#define STACK_SIZE (2*1024*1024)
#define NUM_PROC 20
#define PROC_UNUSED 0
#define PROC_INIT 1
#define PROC_RUNNING 2
#define PROC_READY 3
#define PROC_SLEEP 4
#define PROC_KILLED 5

void init_process(void);
struct ProcessControl* get_pc(void);
void yield(void);
void swap(uint64_t *prev, uint64_t next);
void sleep(int wait);
void wake_up(int wait);
void exit(void);
void wait(int pid);
int fork(void);

#endif