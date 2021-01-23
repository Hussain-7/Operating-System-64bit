#include "print.h"
#include "debug.h"
#include "lib.h"
#include "stddef.h"
#include "stdbool.h"
#include "memory.h"

static void free_region(uint64_t v, uint64_t e);

static struct FreeMemRegion free_mem_region[50];
static struct Page free_memory;
uint64_t total_mem;
uint64_t free_mem;
static uint64_t memory_end;
extern char end;
int free_region_count = 0;
void init_memory(void)
{
    //variable count indicates how many memory regions we have
    //here it is 4 bytes since we use fixed width data type
    int32_t count = *(int32_t*)0x9000;
    //stores the size of free memry we can use in the system
    struct E820 *mem_map = (struct E820*)0x9008;	
    //is used to store actual number of free memory region
    free_region_count = 0;
    ASSERT(count <= 50);
    free_region_count = 0;
	for(int32_t i = 0; i < count; i++) {        
        if(mem_map[i].type == 1) {			
            free_mem_region[free_region_count].address = mem_map[i].address;
            free_mem_region[free_region_count].length = mem_map[i].length;
            total_mem += mem_map[i].length;
            free_region_count++;
        }
        //This is the memory maps
        // printk("%x  %uKB  %u\n",mem_map[i].address,mem_map[i].length/1024,(uint64_t)mem_map[i].type);
	
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
    // printk("%x\n",memory_end);
}

uint64_t get_total_memory(void)
{
    //simply returning memory in mb format
    return total_mem;
}

uint64_t get_free_memory(void)
{
    // init_memory();
    free_mem=0;
    for (int i = 0; i < free_region_count; i++) {                  
        free_mem+= free_mem_region[i].length;  
    }
    return free_mem;
}

uint64_t get_used_memory(void)
{
    // init_memory();
    uint64_t used_memory=0;
    used_memory=total_mem-get_free_memory();
    return used_memory;
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

uint64_t setup_kvm(void)
{
    uint64_t page_map = (uint64_t)kalloc();
    if(page_map != 0){
        memset((void*)page_map, 0, PAGE_SIZE);        
        //using PTE_P|PTE_W in last parameter of map_pages function we specify that kernel memory is readable,writeable and not accessible by the user applications
        if(!map_pages(page_map, KERNEL_BASE, memory_end, V2P(KERNEL_BASE), PTE_P|PTE_W))
        {
            free_vm(page_map);
            page_map=0;
        }

    }
    return page_map;

   
}

//Setting up Kernel Virtual Memory
void init_kvm(void)
{
    uint64_t page_map = setup_kvm();
    //we use assert here because we are in the kernel initialization stage.if operation fails we stop the system
    ASSERT(page_map !=0);
    switch_vm(page_map);
}

//Setting up User Virtual Memory
//Base of the virtual memory for user space is 0x400000
//We map only one page for user progerms which mean the code,data and stack of the programs are on same 2mb page
bool setup_uvm(uint64_t map, uint64_t start, int size)
{
    bool status = false;
    //we allocate a page which will be used to store data nad function of a program
    void *page = kalloc();

    if (page != NULL) {
        //initializing the page
        memset(page, 0, PAGE_SIZE);
        //map - pml4 table address
        //0x400000 - start address of virtual space we map 
        //0x400000+PAGE_SIZE -end address of virtual space is this since we only map one page 
        //v2P(page) - It is the base of physical age we want to map into so we use v2p to convert page to physical address
        //Last one - is the attribute bits which we set all 3 to one
        status = map_pages(map, 0x400000, 0x400000+PAGE_SIZE, V2P(page), PTE_P|PTE_W|PTE_U);
        if (status == true) {
            //if page is successfully created we just copy the data to the page
            memcpy(page, (void*)start, size);
        }
        else {
            //since we just map one page and if operation fails it means
            //mapping is not done when we call free_vm function.hence this page will not be cleared
            //therefore we use kfree to manually free the page
            kfree((uint64_t)page);
            free_vm(map);
        }
    }
    
    return status;
}



//This function does exactly the reverse process of mapping pages
void free_pages(uint64_t map, uint64_t vstart, uint64_t vend)
{
    //The index will be used to locate the correct entry in th page directory table
    unsigned int index; 

    //we call the free pages function assuming that vstart and vend is page aligned 
    //But just in case its not we use asssert call to prevent frrom going ahead if thats not the case
    ASSERT(vstart % PAGE_SIZE == 0);
    ASSERT(vend % PAGE_SIZE == 0);

    do {
        //In this function this time we are passing 0 to alloc so it just returns null if page does not exist
        PD pd = find_pdpt_entry(map, vstart, 0, 0);

        if (pd != NULL) {
            index = (vstart >> 21) & 0x1FF;
            if(pd[index] & PTE_P){           
                kfree(P2V(PTE_ADDR(pd[index])));
                pd[index] = 0;
            }
        }

        vstart += PAGE_SIZE;
    } while (vstart+PAGE_SIZE <= vend);
}

static void free_pdt(uint64_t map)
{
    //The map_entry points to pml4 table
    PDPTR *map_entry = (PDPTR*)map;

    //Since each entry in pml4 table points to a page directory pointer tables
    //Therefore we can have 512 pdp table
    for (int i = 0; i < 512; i++) {
        //we are checking present bit of each entry if present then we further process
        if ((uint64_t)map_entry[i] & PTE_P) {       
            //since this entry points to pd table we convert it to virtual address and set to pdtr pointer     
            PD *pdptr = (PD*)P2V(PDE_ADDR(map_entry[i]));
            
             //Since each entry in pdp table points to a page directory tables
            //Therefore we can have 512 pd table
            for (int j = 0; j < 512; j++) {

                if ((uint64_t)pdptr[j] & PTE_P) {

                    //free the page
                    kfree(P2V(PDE_ADDR(pdptr[j])));
                    //clear this entry
                    pdptr[j] = 0;
                }
            }
        }
    }
}

static void free_pdpt(uint64_t map)
{
    //Same logic as free_pdt function but in this we loop through just the pdp table since we dont free pdpt table in this function 
    PDPTR *map_entry = (PDPTR*)map;
    for (int i = 0; i < 512; i++) {
        if ((uint64_t)map_entry[i] & PTE_P) {          
            kfree(P2V(PDE_ADDR(map_entry[i])));
            map_entry[i] = 0;
        }
    }
}

static void free_pml4t(uint64_t map)
{
    //map is the address of pml4 table so we can directly use it to free table
    kfree(map);
}
//free virtual memory function is actually be required when we build process and create virtual memory
// and then when process exits to clear vm we use this function
//for that purpose we free the physical pages in user space aswell as the page translation tables

void free_vm(uint64_t map)
{   
    free_pages(map,0x400000,0x400000+PAGE_SIZE);
    free_pdt(map);
    free_pdpt(map);
    free_pml4t(map);
}

