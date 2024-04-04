#include "user_lib.h"
#include "util/types.h"

int main(int argc,char*argv[]){
    if(argc<=0){
        setstatus(-1);
        printu("Need Arguments!!!\n");
    }
    else{
        for(int i=0;i<argc;i++){
            int ret=unlink_u(argv[i]);
            if(!ret) printu("Unlink %s Success!\n",argv[i]);
            else{setstatus(-1);printu("Unlink %s Failed\n",argv[i]);}
        }
    }
    exit(0);
    return 0;
}