/*
 * This app starts a very simple shell and executes some simple commands.
 * The commands are stored in the hostfs_root/shellrc
 * The shell loads the file and executes the command line by line.                 
 */
#include "user_lib.h"
#include "string.h"
#include "util/types.h"
#define max_para_num 8
enum commandtype{
  COMMON,
  FIRSTNEXT,
  PIPE,
  SAMETIME,
  AND,
  OR
};
typedef struct input_i
{
  char* str;
  struct input_i* last;
  struct input_i* next;
}instruction;
struct input_i first_input={"no more instructions",NULL,NULL}; 

typedef struct single_command
{ 
  bool valid;
  char commad[30];
  char para[max_para_num][30];
  int paranum;
}onecom;
onecom coms[10];
int comtype=COMMON;
int comnum=0;
char* spilted_buf[10];
int input_offset=0;
int input_cursor=0;
char inputbuf[512];
char tempbuf[256];
char input[20][100];
int inputnum=0;
char *paraaddr[max_para_num];
void sentence2command(char* buf,char** spi_buf,int *comnum);
void tip(){
  char path[40];
  read_cwd(path);
  printu("\33[34m%s\33[37m",path);
  printu(">>>");
}
int beginwith(char* s1,char* s2){
  int j=0,k=0;
  for(j=0,k=0;*(s2+k)!='\0';j++,k++){
    if(*(s1+j)!=*(s2+k)) return 0;
  }
  return 1;
}
int endwith(char *s1,char* s2){
  int l1=strlen(s1);
  int l2=strlen(s2);
  for(char* p=s2;*p!='\0';p++,s1++){
    if(*(s1+l1-l2)!=*p) return 0;    
  }
  return 1;
}
void splitString(char* input, char* delimiter, char** output, int* outputSize) {
    int i = 0;
    char* token = strtok(input, delimiter);
 
    while (token != NULL) {
        output[i] = token;
        i++;
        token = strtok(NULL, delimiter);
    }
 
    *outputSize = i;
}
bool parse_string(char *com,onecom* p){
  char de[2]=" ";
  char* token;
  memset(p,0,sizeof(onecom));
  token=strtok(com,de);
  if(token==NULL) {p->valid=0;return 0;}
  strcpy(p->commad,token);
  while (token!=NULL)
    {
      token=strtok(NULL,de);
      if(token==NULL) break;
      if(p->paranum==max_para_num) {
        printu("too many arguments!!\n");
      }
      strcpy(p->para[p->paranum],token);
      p->paranum++;
    }
    p->valid=1;
    return 1;
}

int execute_command(onecom* p,int others){
  // printu("p->command %s\n",p->commad);
  setstatus(1);
  if(endwith(p->commad,".sh")){
    int fd=open(p->commad,O_RDONLY);
    if(fd<0){
      setstatus(-1);
      printu("%s:No Such File!",p->commad);
    }
    else{
    struct istat stat;
    stat_u(fd, &stat); 
    printu("%d\n",stat.st_size);
    char* filebuf=better_malloc(stat.st_size+1);
    read_u(fd,filebuf,stat.st_size);
    close(fd);
    char* filesens[100];
    int filesennum=0;
    splitString(filebuf,"\n",filesens,&filesennum);
    for(int i=0;i<filesennum;i++){
      tip();
      printu("%s\n",filesens[i]);
      sentence2command(filesens[i],spilted_buf,&comnum);
    }
    }
  }
  else if(!strcmp(p->commad,"cd")){
    if(p->paranum<=0){
      printu("Need Arguments!!!\n");
      setstatus(-1);
    }
    else{
      if (change_cwd(p->para[0]) != 0){
    printu("No Such Path!!!\n");
    setstatus(-1);
    }
    }
    return -1;
  }
  else if(!strcmp(p->commad,"exit")){
    shutdown();
    print_with_hartid("Exiting\n");
    exit(0);
  }
  else if(!strcmp(p->commad,"pwd")){
    char path[30];
    read_cwd(path);
    printu("cwd:%s\n", path);
    return 1;
  }
  else if(!strcmp(p->commad,"histroy")){
    int i=1;
    for(instruction* in=first_input.next;in!=NULL;in=in->next){
      printu("%d %s\n",i,in->str);
      i++;
    }
  }
  else if(!strcmp(p->commad,"ps")){
    ps();
  }
  else{
    int fd=open(p->commad,O_RDONLY);
    if(fd<0){
      char name[50]="/bin/";
      strcpy(name+strlen(name),p->commad);
      // printu("%s\n",name);
      int fd2=open(name,O_RDONLY);
      if(fd2>=0){
        
        strcpy(p->commad,name);
        close(fd2);
      }
      else{
        // printu("fd2 is %d\n",fd2);
        setstatus(-1);
        printu("%s:No Such Command!!!\n",p->commad);
        return 1;
      }
    }
    else{
      close(fd);
    }
     int pid=fork();
    // printu("pid=%d\n",pid);
    if(pid==0){
    for(int i=0;i<p->paranum;i++){
      paraaddr[i]=p->para[i];
      // printu("%p %s\n",paraaddr[i],paraaddr[i]);
    }

    int ret=exec(p->commad,p->paranum,paraaddr);
    if(ret==0){
      setstatus(-1);
    printu("exec failed");
  }
  }
    if(others){
      // printu("puttask\n");
      puttask(pid);
    }
    else{
    wait(pid);
    
    // kill(pid);
    }
  }
  return 1;
}


void scan(){
  // printu("%d  %d  %p\n",input_offset,input_cursor,inputbuf);
  tip();
  memset(inputbuf,0,sizeof(inputbuf));
  input_cursor=0;
  input_offset=0;
  instruction* newins=better_malloc(sizeof(instruction));
  instruction* p;
  instruction* current=newins;
  // printu("new in %p\n",newins);
  for(p=&first_input;p->next!=NULL;p=p->next);
  // {printu("%p %p %p\n",p->next,p->last,p->str);
  //   printpa(p);
  // }
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
      char names[50][30];
      int nameslen=0;
      nameslen=0;
      memset(names,0,sizeof(names));
      int i,j,k=0;
      if(input_cursor==0) goto onecharover;
      for(i=input_cursor-1;i>=0&&inputbuf[i]!=' '&&inputbuf[i]!=';'&&inputbuf[i]!='|'&&inputbuf[i]!='&';i--);
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
      if(endwith(names[i],"/"))
      printu("\33[34m%s\33[37m \t",names[i]);
      else printu("%s \t",names[i]);
    }
    printu("\n");
    tip();
    printu("%s",inputbuf);
    for(int i=input_offset;i>input_cursor;i--)printu("\b");
    }
    }
    else if(c==0){
      goto onecharover;
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

void sentence2command(char* buf,char** spi_buf,int *comnum){
    splitString(buf,"&&",spi_buf,comnum);
    if(*comnum>1) {comtype=AND; goto real_com;}
    splitString(buf,"||",spi_buf,comnum);
    if(*comnum>1) {comtype=OR; goto real_com;}
    splitString(buf,";",spi_buf,comnum);
    if(*comnum>1) {comtype=FIRSTNEXT; goto real_com;}
    splitString(buf,"&",spi_buf,comnum);
    if(*comnum>1) {comtype=SAMETIME; goto real_com;}
    splitString(buf,"|",spi_buf,comnum);
    if(*comnum>1) {comtype=PIPE; goto real_com;}
    comtype=COMMON;
real_com:
    if(*comnum>10) {printu("Too Many Commands!!\n"); return;}
    switch (comtype)
    {
    case COMMON:
      for(int i=0;i<*comnum;i++){
        if(parse_string(spi_buf[i],&coms[i]))
        execute_command(&coms[i],0);
      }
      break;
    case FIRSTNEXT:
    for(int i=0;i<*comnum;i++){
        // printu("%d %s\n",i,spilted_buf[i]);
        if(parse_string(spi_buf[i],&coms[i]))
        execute_command(&coms[i],0);
      }
      break;
    case PIPE:
      break;
    case SAMETIME:
      for(int i=*comnum-1;i>=0;i--){
        // printu("i=%d\n%s\n",i,spi_buf[i]);
        if(parse_string(spi_buf[i],&coms[i]))
        execute_command(&coms[i],i==0?0:1);
      }
      while (checktask())
      {
        int pid=gettask();
        if(pid>=0)
        wait(pid);
      }
      break;
    case AND:
      setstatus(0);
      for(int i=0;i<*comnum;i++){
        if(getstatus()<0) break;
        if(parse_string(spi_buf[i],&coms[i]))
        execute_command(&coms[i],0);
      }
      break;
    case OR:
      setstatus(0);
      for(int i=0;i<*comnum;i++){
        // printu("%d %s\n",i,spilted_buf[i]);
        if(getstatus()>0) break;
        if(parse_string(spi_buf[i],&coms[i]))
        execute_command(&coms[i],0);
      }
      break;
    default:
      break;
    } 
}
void varInit(){
  first_input.last=NULL;
  first_input.next=NULL;
}
int main(int argc, char *argv[]) {
  LibInit();
  varInit();
  printu("\n======== Shell Start ========\n\n");
  while(1){
    
    scan();
    sentence2command(inputbuf,spilted_buf,&comnum);
  once_over:;
  }
  print_with_hartid("Exiting\n");
  exit(0);
  return 0;
}

