#include "trap.h"
#include "print.h"
#include "syscall.h"
#include "process.h"
#include "debug.h"

static struct IdtPtr idt_pointer;
static struct IdtEntry vectors[256];
static uint64_t ticks;

//It takes 3 params address og idt entry,the address of handler which we defined in assembly
//and the attribute of the idt entry
static void init_idt_entry(struct IdtEntry *entry, uint64_t addr, uint8_t attribute)
{
    //Initializing values for Hardware Interrupt
    entry->low = (uint16_t)addr;
    entry->selector = 8;
    entry->attr = attribute;
    entry->mid = (uint16_t)(addr>>16);
    entry->high = (uint32_t)(addr>>32);
}

void init_idt(void)
{
    init_idt_entry(&vectors[0],(uint64_t)vector0,0x8e);
    init_idt_entry(&vectors[1],(uint64_t)vector1,0x8e);
    init_idt_entry(&vectors[2],(uint64_t)vector2,0x8e);
    init_idt_entry(&vectors[3],(uint64_t)vector3,0x8e);
    init_idt_entry(&vectors[4],(uint64_t)vector4,0x8e);
    init_idt_entry(&vectors[5],(uint64_t)vector5,0x8e);
    init_idt_entry(&vectors[6],(uint64_t)vector6,0x8e);
    init_idt_entry(&vectors[7],(uint64_t)vector7,0x8e);
    init_idt_entry(&vectors[8],(uint64_t)vector8,0x8e);
    init_idt_entry(&vectors[10],(uint64_t)vector10,0x8e);
    init_idt_entry(&vectors[11],(uint64_t)vector11,0x8e);
    init_idt_entry(&vectors[12],(uint64_t)vector12,0x8e);
    init_idt_entry(&vectors[13],(uint64_t)vector13,0x8e);
    init_idt_entry(&vectors[14],(uint64_t)vector14,0x8e);
    init_idt_entry(&vectors[16],(uint64_t)vector16,0x8e);
    init_idt_entry(&vectors[17],(uint64_t)vector17,0x8e);
    init_idt_entry(&vectors[18],(uint64_t)vector18,0x8e);
    init_idt_entry(&vectors[19],(uint64_t)vector19,0x8e);
    init_idt_entry(&vectors[32],(uint64_t)vector32,0x8e);
    init_idt_entry(&vectors[39],(uint64_t)vector39,0x8e);
    //in case of this interrup we set dpl to 3 instead of zero because we fire interrupt in ring3
    init_idt_entry(&vectors[0x80],(uint64_t)sysint,0xee);

    idt_pointer.limit = sizeof(vectors)-1;
    idt_pointer.addr = (uint64_t)vectors;
    load_idt(&idt_pointer);
}

uint64_t get_ticks(void)
{
    return ticks;
}

static void timer_handler(void)
{
    ticks++;
    wake_up(-1);
} 

//All interrupts are handled here
//its parameter is actually a stack pointer which we have defined in trap assembly file
//The reason we have the trap frame structure in the process is that in our system, we have two entry points
//when we switch from ring3 to ring0. One is through interrupts,another is through exceptions.
//Since we have handled them in the same function handler, this is actually the only entry point.
//Which means the function will be called when we jump from ring3 to ring0.

void handler(struct TrapFrame *tf)
{
    unsigned char isr_value;

    switch (tf->trapno) {
        //here we are dealing with only two interrupts the timer and spurious interrupt as 
        //the trap no shows
        case 32:
            timer_handler();
            eoi();
            break;
            
        case 39:
            isr_value = read_isr();

            //To check if it is a real interrupt.if bit 7 is set then it is real interrupt
            if ((isr_value&(1<<7)) != 0) {
                eoi();
            }
            break;
        case 0x80:
        {
            system_call(tf);
            break;
        }
        default:
            {
                //reminder lower 3 bits stores the current priveledge level
                //the virtual address which we try to access which casue exception.This  is stored in 
                printk("[Error %d at ring %d] %d:%x %x",tf->trapno,(tf->cs &3),tf->errorcode,read_cr2(),tf->rip);
                while (1) { }
            }
           
    }
    //We use timer interrupt to switch between process 
    //Since in the process set process entry we set rflags to to 202 which also implies that interrupts are enabled in user mode.
    //So timer interrupt will also be be processed when we are in the user program and will be handled by trap handler.
    //Therfore when this condition meets we call yield function to make current process give up cpu resources and choose another process
    if(tf->trapno == 32)
    {
        yield();
    }
}