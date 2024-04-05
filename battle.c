#include <stdio.h>

// for direct gcc, use Makefile and type "make" to do default make with default port
// do "make PORT=x" to make with port x
#ifndef PORT
  #define PORT 57076
#endif


int main() {
    printf("%d", PORT);
    return 0;
}