#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"

int main(int argc, char *argv[]){
    while (1)
    {
        char c;
        c=getcharu();
        printu("\n%d\n",c);
    }
    
    
    exit(0);
    return 0;
}