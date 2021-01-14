#include "lib.h"

int main(void)
{
    char *p=(char*)0xffff800000200200;
    *p = 1;
    printf("process2\n");
    //Since interrupt is checked every 10ms so passing 100 would mean sleep for around 1sec
    sleepu(100);
    return 0;
}