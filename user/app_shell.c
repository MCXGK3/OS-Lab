/*
 * This app starts a very simple shell and executes some simple commands.
 * The commands are stored in the hostfs_root/shellrc
 * The shell loads the file and executes the command line by line.                 
 */
#include "user_lib.h"
#include "string.h"
#include "util/types.h"
#define max_para_num 7

int main(int argc, char *argv[]) {
  char buf[1024];
  char de[2]=" ";
  printu("\n======== Shell Start ========\n\n");
  while(1){
    char* token;
    char commad[30];
    char para[max_para_num][30];
    int paranum=0;
    scanu(buf);
    token=strtok(buf,de);
    strcpy(commad,token);
    while (token!=NULL)
    {
      token=strtok(NULL,de);
      if(token==NULL) break;
      if(paranum==max_para_num) {
        printu("too many arguments!!\n");
        goto once_over;
      }
      strcpy(para[paranum],token);
      paranum++;
    }
    printu("command:%s\n",commad);
    for(int i=0;i<paranum;i++){
      printu("para%d:%s\n",i+1,para[i]);
    }
    
  once_over:;
  }
  exit(0);
  return 0;
}