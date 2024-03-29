#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
    while (1)
    {
        while (!checktask())
        {
        }
        printu("checktask\n");
        int pid=gettask();
        printu("pid=%d\n",pid);
        wait(pid);
    }
    
    exit(0);
    return 0;
}
