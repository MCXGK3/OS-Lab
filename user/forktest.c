#include "user_lib.h"
#include "util/types.h"
#include "util/string.h"

int main(int argc,char* argv[]) {
    for(int i=0;i<100;i++){
        int pid=fork();
        if(pid==0){
            LibInit();
            int* x=better_malloc(4096);
            x[0]=i;
            print_with_hartid("x[0]=%d\n",i);
            exec("/bin/echo",0,NULL);
            break;
        }
        // else{
        //     wait(pid);
        // }

    }
  
  exit(0);
  return 0;
}