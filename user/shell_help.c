#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
    while (1)
    {
        int t;
        while (!(t=checktask()))
        {
        // print_with_hartid("checking\n");
        }
        if(t==-1) break;
        // print_with_hartid("checktask\n");
        int pid=gettask();
        // print_with_hartid("pid=%d\n",pid);
        if(pid>=0) wait(pid);
        // kill(pid);
        
    }
    print_with_hartid("Exiting\n");
    
    exit(0);
    return 0;
}
