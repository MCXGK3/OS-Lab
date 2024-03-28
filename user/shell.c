/*
 * This app starts a very simple shell and executes some simple commands.
 * The commands are stored in the hostfs_root/shellrc
 * The shell loads the file and executes the command line by line.                 
 */
#include "user_lib.h"
#include "string.h"
#include "util/types.h"
#define max_para_num 8
typedef struct input_i
{
  char* str;
  struct input_i* last;
  struct input_i* next;
}instruction;
struct input_i first_input={"no more instructions",NULL,NULL}; 
int input_offset=0;
int input_cursor=0;
char inputbuf[512];
char tempbuf[256];
char input[20][100];
int inputnum=0;
int beginwith(char* s1,char* s2){
  int j=0,k=0;
  for(j=0,k=0;*(s2+k)!='\0';j++,k++){
    if(*(s1+j)!=*(s2+k)) return 0;
  }
  return 1;
}

void scan(){
  // printu("%d  %d  %p\n",input_offset,input_cursor,inputbuf);
  printu(">>>");
  memset(inputbuf,0,sizeof(inputbuf));
  input_cursor=0;
  input_offset=0;
  instruction* newins=better_malloc(sizeof(instruction));
  instruction* p;
  instruction* current=newins;
  printu("new in %p\n",newins);
  for(p=&first_input;p->next!=NULL;p=p->next)
  {printu("%p %p %p\n",p->next,p->last,p->str);
    printpa(p);
  }
  p->next=newins;
  newins->last=p;
  newins->next=NULL;
  newins->str=inputbuf;
  char c=getcharu();
  while (c!='\n')
  { 
    if(c==127){
      if(input_offset<=0||input_cursor<=0){
      }
      else
      {
      printu("\b");
      for(int i=input_cursor-1;i<input_offset;i++){
        inputbuf[i]=inputbuf[i+1];
        printu("%c",inputbuf[i]);
      }
      inputbuf[input_offset-1]='\0';
      printu(" ");
      input_cursor--;
      input_offset--;
      for(int i=strlen(inputbuf)+1;i>input_cursor;i--){
        printu("\b");
      }
      }
    }
    else if(c==27){
      c=getcharu();
      if(c==91){
        c=getcharu();
        if(c>68||c<65){
          inputbuf[input_offset++]=c;
        input_cursor++;
        printu("[%c",c);
        }
        else{
          switch (c)
          {
          case 65:
            if(current==newins){
              strcpy(tempbuf,inputbuf);
              newins->str=tempbuf;
            }
            if(current->last==NULL) goto onecharover;
            current=current->last;
            for(int i=input_cursor;i>0;i--)printu("\b");
            for(int i=0;i<input_offset;i++) printu(" ");
            for(int i=input_offset;i>0;i--)printu("\b");
            strcpy(inputbuf,current->str);
            input_offset=strlen(inputbuf);
            input_cursor=input_offset;
            printu("%s",inputbuf);
            /* code */
            break;
          case 66:
            if(current->next==NULL) goto onecharover;
            current=current->next;
            for(int i=input_cursor;i>0;i--)printu("\b");
            for(int i=0;i<input_offset;i++) printu(" ");
            for(int i=input_offset;i>0;i--)printu("\b");
            strcpy(inputbuf,current->str);
            input_offset=strlen(inputbuf);
            input_cursor=input_offset;
            printu("%s",inputbuf);
            break;
          case 68:
            if(input_cursor>0){
              printu("\b");
              input_cursor--;}
            break;
          case 67:
            if(input_cursor<strlen(inputbuf)){
              printu("%c",inputbuf[input_cursor++]);
            }
          default:
            break;
          }
        }
      }
    }
    else if(c==9){
      char de[2]="/";
      char tempname[30]="";
      char dirname[30]="";
      int diroffset=0;
      char basename[30]="";
      char names[30][30];
      int nameslen=0;
      nameslen=0;
      memset(names,0,sizeof(names));
      int i,j,k=0;
      if(input_cursor==0) goto onecharover;
      for(i=input_cursor-1;i>=0&&inputbuf[i]!=' ';i--);
      for(j=i+1;j<input_cursor;j++){
        tempname[k++]=inputbuf[j];
      } 
      tempname[k]='\0';
      if(strlen(tempname)==0) goto onecharover;
      if(tempname[strlen(tempname)-1]=='/'){
        strcpy(dirname,tempname);
        basename[0]='\0';
      }
      else{
      if(beginwith(tempname,"/")){
        strcpy(dirname,"/");
        diroffset+=1;
      }
      char *token = strtok(tempname, "/");
      char *last_token = NULL;
      while (token != NULL) {
      if(last_token!=NULL){
        strcpy(dirname+diroffset,last_token);
        diroffset+=strlen(last_token);
        dirname[diroffset++]='/';
      }
      last_token = token;
      token = strtok(NULL, "/");
      }
    strcpy(basename, last_token);
      }
    if(dirname[0]=='\0')strcpy(dirname,"./");
    // printu("dir name is %s\n",dirname);
    // printu("base name is %s\n",basename);

    struct dir Dir;
    int dir=opendir_u(dirname);
    if(dir==-1) goto onecharover;
    while (readdir_u(dir,&Dir)==0)
    {
      if(beginwith(Dir.name,basename)){
        if(Dir.type==DIR_I){strcpy(Dir.name+strlen(Dir.name),"/");}
        strcpy(names[nameslen++],Dir.name);
      }
    }
    closedir_u(dir);
    if(nameslen==1){
      if(!strcmp(names[0],basename))goto onecharover;
      char temp[256];
      strcpy(temp,inputbuf+input_cursor);
      strcpy(inputbuf+input_cursor,names[0]+strlen(basename));
      strcpy(inputbuf+strlen(inputbuf),temp);
      for(int i=input_cursor;i<=input_offset;i++){
        printu(" ");
      }
      for(int i=input_offset;i>=0;i--){
        printu("\b");
      }
      printu("%s",inputbuf);
      for(int i=input_offset;i>input_cursor;i--)printu("\b");
      input_cursor+=strlen(inputbuf)-input_offset;
      input_offset=strlen(inputbuf);

    }
    else if(nameslen==0) goto onecharover;
    else{
    printu("\n");
    for(int i=0;i<nameslen;i++){
      printu("%s ",names[i]);
    }
    printu("\n");
    printu(">>>%s",inputbuf);
    for(int i=input_offset;i>input_cursor;i--)printu("\b");
    }
    }
    else{
      // printu("%d ",c);
    // printu("input %c\n",c);
    // printu("offset is %p\ncursor is %p\n",&input_offset,&input_cursor);
    char temp1,temp2;
    int i;
    for(i=input_cursor;i<=input_offset;i++){
      temp1=inputbuf[i];
      inputbuf[i]=c;
      printu("%c",c);
      c=temp1;
    }
    inputbuf[i]='\0';
    input_offset++;
    input_cursor++;
    for(i=input_offset;i>input_cursor;i--)printu("\b");
    // 上 27 91 65  下 27 91 66 右 27 91 67 左 27 91 68
    // printu("%p\n",inputbuf);
    // printpa((int*)inputbuf);
    }
    // refresh();
onecharover:
    c=getcharu();
  }
  if(!strcmp(newins->last->str,inputbuf)||strlen(inputbuf)<=0){newins->last->next=NULL; better_free(newins);}
  else{
    int l=strlen(inputbuf);
    char* newstr=better_malloc(l+1);
    strcpy(newstr,inputbuf);
    newins->str=newstr;
    newins->next=NULL;
    // printu("\n%s %p %p\n",newins->str,newins->last,newins->next);
  }
  printu("\n");
  return;
}

int main(int argc, char *argv[]) {
  char de[2]=" ";
  char test[4096];
  LibInit();
  printu("\n======== Shell Start ========\n\n");
  while(1){
    char* token;
    char commad[30];
    char para[max_para_num][30];
    char *paraaddr[max_para_num];
    int paranum=0;
    //scanu(buf);
    scan();
    token=strtok(inputbuf,de);
    if(token==NULL) continue;
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
      paraaddr[paranum]=para[paranum];
      paranum++;
    }
    // printu("command:%s\n",commad);
    // for(int i=0;i<paranum;i++){
    //   printu("para%d:%s\n",i+1,para[i]);
    // }
  if(!strcmp(commad,"cd")){
    if(paranum<=0){
      printu("Need Arguments!!!\n");
    }
    else{
      if (change_cwd(para[0]) != 0)
    printu("No Such Path!!!\n");
    }
    goto once_over;
  }
  else if(!strcmp(commad,"exit")){
    break;
  }
  else if(!strcmp(commad,"pwd")){
    char path[30];
    read_cwd(path);
    printu("cwd:%s\n", path);
    goto once_over;
  }
  int pid=fork();
  if(pid==0){
  int ret=exec(commad,paranum,paraaddr);
  if(ret==0){
    printu("exec failed");
  }
  }
  else{
    // printu("wait\n");
    wait(pid);
  }
  once_over:;
  }
  exit(0);
  return 0;
}

