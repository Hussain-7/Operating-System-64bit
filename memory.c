#include "print.h"
#include "debug.h"
#include "lib.h"
#include "stddef.h"
#include "stdbool.h"
#include "memory.h"

static void free_region(uint64_t v, uint64_t e);

static struct FreeMemRegion free_mem_region[50];
static struct Page free_memory;
static uint64_t memory_end;
uint64_t page_map;
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

//This function finds specific pml4 table entry which then points to  the page directory pointer table

static PDPTR find_pml4t_entry(uint64_t map, uint64_t v, int alloc, uint32_t attribute)
{

    PDPTR *map_entry = (PDPTR*)map;
    PDPTR pdptr = NULL;
    unsigned int index = (v >> 39) & 0x1FF;

    if ((uint64_t)map_entry[index] & PTE_P) {
        pdptr = (PDPTR)P2V(PDE_ADDR(map_entry[index]));       
    } 
    else if (alloc == 1) {
        pdptr = (PDPTR)kalloc();          
        if (pdptr != NULL) {     
            memset(pdptr, 0, PAGE_SIZE);     
            map_entry[index] = (PDPTR)(V2P(pdptr) | attribute);           
        }
    } 

    return pdptr;    
}

//This function is used to find the entry in the page directory pointer table which points to page directory pointer table 
//
static PD find_pdpt_entry(uint64_t map, uint64_t v, int alloc, uint32_t attribute)
{
    //Function Parameters
    //map parameter is the pml4 table,v is the virual address and alloc indicates whether or not we will create a page if it does not exist
    
    PDPTR pdptr = NULL;
    PD pd = NULL;
    //since the pdtp entries index value starts from bit 30 and is 9 bit long so we clear the bit before and after it
    unsigned int index = (v >> 30) & 0x1FF;

    //we call this function get the page directory pointer table
    pdptr = find_pml4t_entry(map, v, alloc, attribute);
    if (pdptr == NULL)
        return NULL;
       
    //if present attribute is set to 1 then it means that value in the entry points to the next level tabel  which is page directory tbale
    if ((uint64_t)pdptr[index] & PTE_P) {      
        pd = (PD)P2V(PDE_ADDR(pdptr[index]));      
    }
    else if (alloc == 1) {
        //In this case we allocate a new page and set entry to make it point to this page
        pd = (PD)kalloc();  
        
        if (pd != NULL) {    
            memset(pd, 0, PAGE_SIZE);       
            pdptr[index] = (PD)(V2P(pd) | attribute);
        }
    } 

    return pd;
}

//Then in the page directory table we can set corresponding entry to map the addresses to the the physical page
//and for that purpose we use map_pages functions

bool map_pages(uint64_t map, uint64_t v, uint64_t e, uint64_t pa, uint32_t attribute)
{
    //v start and vend are used to save the aligned virtual addresses
    //Remeber here we are alogning them to 2mb pages
    //So if we get un aligned start virtual address then we need to map the page where the start address is located 
    //so in PA_down function we align the address to the previous 2mb page so that we ccan include the start address
    //vice versa incase of end address
    uint64_t vstart = PA_DOWN(v);
    uint64_t vend = PA_UP(e); 
    //pd is used to set the page directory entry.
    PD pd = NULL;
    //index is used to locate specific entry in the table
    unsigned int index;

    //Checks the start and end of the region
    ASSERT(v < e);
    //This check would make sure that the physical address we want to map is page aligned
    ASSERT(pa % PAGE_SIZE == 0);
    //This checks makes sure that the end of physical address is not outside the range of 1gb memory
    ASSERT(pa+vend-vstart <= 1024*1024*1024);

    do {
        pd = find_pdpt_entry(map, vstart, 1, attribute);    
        if (pd == NULL) {
            return false;
        }

        //Since the directory we want here is page directory entry whose index value (9 bits in total) starting from bit 21
        //So to get bit 21 we shift start address right 21 bits 
        //And then also clear all bit except the lower 9 by using and operation
        index = (vstart >> 21) & 0x1FF;

        //Here we clear all the bit of entry selected except the present bit and check if it is set to zero
        //since if it is 1 it would mean that we allow remap to the used pages which we donot want to allow
        ASSERT(((uint64_t)pd[index] & PTE_P) == 0);

        //We also add attribute entry which indicates this is 2mb page entry
        //using this we map a page to physical memory
        pd[index] = (PDE)(pa | attribute | PTE_ENTRY);

        //we move to the next page by adding page size to va and pa
        vstart += PAGE_SIZE;
        pa += PAGE_SIZE;
        //we check if Va is still within memory region then we'll continue else we stop the process if end of region reached 
    } while (vstart + PAGE_SIZE <= vend);
  
    return true;
}


//Switch vm is used to load cr3 register with the new translation table to make the mapping work

void switch_vm(uint64_t map)
{
    load_cr3(V2P(map));
}

//Is used to remap out kernel using 2 mb pages 

static void setup_kvm(void)
{
    page_map = (uint64_t)kalloc();
    ASSERT(page_map != 0);

    memset((void*)page_map, 0, PAGE_SIZE);        
    //using PTE_P|PTE_W in last parameter of map_pages function we specify that kernel memory is readable,writeable and not accessible by the user applications
    bool status = map_pages(page_map, KERNEL_BASE, memory_end, V2P(KERNEL_BASE), PTE_P|PTE_W);
    ASSERT(status == true);
}


void init_kvm(void)
{
    setup_kvm();
    switch_vm(page_map);
    printk("memory manager is working now");
}
