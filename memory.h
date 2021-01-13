#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

struct E820 {
    uint64_t address;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

struct FreeMemRegion {
    uint64_t address;
    uint64_t length;
};

struct Page {
    struct Page* next;
};

typedef uint64_t PDE;
typedef PDE* PD;
//PDPTR points to page directory and the PD points to page directory entry
typedef PD* PDPTR;


//The top 4 defined macros are attributes of the table entries
#define PTE_P 1         //sets 1st bit to 1
#define PTE_W 2         //sets 2nd bit to 1
#define PTE_U 4         //sets 3rd bit to 1
#define PTE_ENTRY 0x80  //sets 7th bit to 1
#define KERNEL_BASE 0xffff800000000000
#define PAGE_SIZE (2*1024*1024)

// next two macros set the address to 2mb boundary
//this will align address to next 2mb boundary
#define PA_UP(v) ((((uint64_t)v + PAGE_SIZE-1) >> 21) << 21)
//this will align address to previous 2mb boundary
#define PA_DOWN(v) (((uint64_t)v >> 21) << 21)
#define P2V(p) ((uint64_t)(p) + KERNEL_BASE)
#define V2P(v) ((uint64_t)(v) - KERNEL_BASE)
//the entries in pml4 pdpt and pdt tables have attributes within the lower 12 bits
//hence we clear the lower 12 bits using the macro below
#define PDE_ADDR(p) (((uint64_t)p >> 12) << 12)
//The page table entry has attributes within first 21 bit
//it can be cleared using the following macro
#define PTE_ADDR(p) (((uint64_t)p >> 21) << 21)


void init_memory(void);
void init_kvm(void);
void switch_vm(uint64_t map);
void* kalloc(void);
void kfree(uint64_t v);
bool map_pages(uint64_t map, uint64_t v, uint64_t e, uint64_t pa, uint32_t attribute);
void load_cr3(uint64_t map);
void free_vm(uint64_t map);
void free_page(uint64_t map, uint64_t v, uint64_t e);
bool setup_uvm(uint64_t map, uint64_t start, int size);
uint64_t setup_kvm(void);

#endif











