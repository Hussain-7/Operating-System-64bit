#include "lib.h"
#include "console.h"
#include "stdint.h"

    
static void cmd_get_total_memory(void)
{
    uint64_t total;
    
    total = get_total_memoryu();
    printf("Total Memory is %dmb\n", total/(1024*1024));
}
static void cmd_get_free_memory(void)
{
    uint64_t total;
    total = get_free_memoryu();
    printf("Total Free Memory is %dmb\n", total/(1024*1024));

}
static void cmd_get_used_memory(void)
{
    uint64_t total;    
    total = get_used_memoryu();
    printf("Total Used Memory is %dmb\n", total/(1024*1024));
}

static void cmd_clear_screen(void)
{
   clear_screenu();
}

static int read_cmd(char *buffer)
{
    char ch[2] = { 0 };
    int buffer_size = 0;

    while (1) {
        ch[0] = keyboard_readu();
        
        if (ch[0] == '\n' || buffer_size >= 80) {
            printf("%s", ch);
            break;
        }
        else if (ch[0] == '\b') {    
            if (buffer_size > 0) {
                buffer_size--;
                printf("%s", ch);    
            }           
        }          
        else {     
            buffer[buffer_size++] = ch[0]; 
            printf("%s", ch);        
        }
    }

    return buffer_size;
}

static int parse_cmd(char *buffer, int buffer_size)
{
    int cmd = -1;

    if (buffer_size == 8 && (!memcmp("totalmem", buffer, 8))) {
        cmd = 0;
    }
    else if (buffer_size == 7 && (!memcmp("freemem", buffer, 7))) {
        cmd = 1;
    }
    else if (buffer_size == 7 && (!memcmp("usedmem", buffer, 7))) {
        cmd = 2;
    }
    else if (buffer_size == 5 && (!memcmp("clear", buffer, 5))) {
        cmd = 3;
    }
    

    return cmd;
}

static void execute_cmd(int cmd)
{ 
    CmdFunc cmd_list[4] = {cmd_get_total_memory,cmd_get_free_memory,cmd_get_used_memory,cmd_clear_screen};
    
    switch (cmd)
    {
    case 0:
         cmd_list[0]();
        break;
    case 1:
         cmd_list[1]();
        break;
    case 2:
         cmd_list[2]();
        break;
    case 3:
        cmd_list[3]();
        break;
    
    default:
        break;
    }
    
}

int main(void)
{
    char buffer[80] = { 0 };
    int buffer_size = 0;
    int cmd = 0;

    while (1) {
        printf("shell# ");
        buffer_size = read_cmd(buffer);

        if (buffer_size == 0) {
            continue;
        }
        
        cmd = parse_cmd(buffer, buffer_size);
        
        if (cmd < 0) {
            printf("Command Not Found!\n");
        }
        else {
            execute_cmd(cmd);             
        }            
    }

    return 0;
}
