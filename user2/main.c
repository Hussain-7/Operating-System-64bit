#include "lib.h"

int main(void)
{
    int64_t counter = 0;

    while (1) {
            printf("process2 %d\n",counter);
            sleepu(100);
    }
    return 0;
}