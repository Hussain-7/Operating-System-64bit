#include "memory.h"
#include "print.h"
#include "debug.h"
#include "stddef.h"

static void free_region(uint64_t v, uint64_t e);

static struct FreeMemRegion free_mem_region[50];
static struct Page free_memory;
static uint64_t memory_end;
extern char end;

void init_memory(void)
{
    //variable count indicates how many memory regions we have
    //here it is 4 bytes since we use fixed width data type
    int32_t count = *(int32_t*)0x9000;
    //stores the size of free memry we can use in the system
    uint64_t total_mem = 0;
    struct E820 *mem_map = (struct E820*)0x9008;	
    //is used to store actual number of free memory region
    int free_region_count = 0;


    ASSERT(count <= 50);

	for(int32_t i = 0; i < count; i++) {        
        if(mem_map[i].type == 1) {			
            free_mem_region[free_region_count].address = mem_map[i].address;
            free_mem_region[free_region_count].length = mem_map[i].length;
            total_mem += mem_map[i].length;
            free_region_count++;
        }
        
        printk("%x  %uKB  %u\n",mem_map[i].address,mem_map[i].length/1024,(uint64_t)mem_map[i].type);
	
    }
    for (int i = 0; i < free_region_count; i++) {                  
        uint64_t vstart = P2V(free_mem_region[i].address);
        uint64_t vend = vstart + free_mem_region[i].length;

        if (vstart > (uint64_t)&end) {
            free_region(vstart, vend);
        } 
        else if (vend > (uint64_t)&end) {
            free_region((uint64_t)&end, vend);
        }       
    }

    //since free_memory.next point to the last page hence by added a page size to address of lastpage we can get memory_end
    memory_end = (uint64_t)free_memory.next+PAGE_SIZE;
    printk("%x\n",memory_end);
}

static void free_region(uint64_t v, uint64_t e)
{
    for (uint64_t start = PA_UP(v); start+PAGE_SIZE <= e; start += PAGE_SIZE) {        
        if (start+PAGE_SIZE <= 0xffff800040000000) {            
           kfree(start);
        }
    }
}

void kfree(uint64_t v)
{
    ASSERT(v % PAGE_SIZE == 0);
    ASSERT(v >= (uint64_t)&end);
    ASSERT(v+PAGE_SIZE <= 0xffff800040000000);

    //converting virtual address to the pointer of type structure page
    struct Page *page_address = (struct Page*)v;
    //this copies the head of list to first 8 bytes of the page
    page_address->next = free_memory.next;
    //this saves the virtual address to free memory
    free_memory.next = page_address;
}

void* kalloc(void)
{
    struct Page *page_address = free_memory.next;

    if (page_address != NULL) {
        ASSERT((uint64_t)page_address % PAGE_SIZE == 0);
        ASSERT((uint64_t)page_address >= (uint64_t)&end);
        ASSERT((uint64_t)page_address+PAGE_SIZE <= 0xffff800040000000);

        //since we are returning page pointed by head now we point the head to thhe next page
        free_memory.next = page_address->next;            
    }
    
    return page_address;
}