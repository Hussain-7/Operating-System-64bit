#include "lib.h"

int main(void)
{
    printf("process2\n");
    //Since interrupt is checked every 10ms so passing 100 would mean sleep for around 1sec
    sleepu(100);
    return 0;
}