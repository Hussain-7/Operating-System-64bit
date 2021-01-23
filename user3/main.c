#include "lib.h"
#include "stdint.h"

int main(void)
{
    int64_t counter = 0;

    while (1) {
        if(counter%5000000==0)
        {
            // printf("\nFrom Process 3");
        }
          if(counter==20000000)
        {
            // printf("\n");
            exitu();
        }
        counter++;
    }
    return 0;
}