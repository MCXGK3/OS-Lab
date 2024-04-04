/*
 * header file to be used by applications.
 */

#ifndef _USER_LIB_H_
#define _USER_LIB_H_
#include "util/types.h"
#include "kernel/proc_file.h"

typedef struct memory_block_t 
{
  struct memory_block_t* next;
  uint64 size;
}memory_block;


int printu(const char *s, ...);
int print_with_hartid(const char* s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();

// added @ lab4_1
int open(const char *pathname, int flags);
int read_u(int fd, void *buf, uint64 count);
int write_u(int fd, void *buf, uint64 count);
int lseek_u(int fd, int offset, int whence);
int stat_u(int fd, struct istat *istat);
int disk_stat_u(int fd, struct istat *istat);
int close(int fd);

// added @ lab4_2
int opendir_u(const char *pathname);
int readdir_u(int fd, struct dir *dir);
int mkdir_u(const char *pathname);
int closedir_u(int fd);

// added @ lab4_3
int link_u(const char *fn1, const char *fn2);
int unlink_u(const char *fn);

int exec(const char *pathname,int n,char **paraname);

int wait(int id);
int scanu(char* buf);
char getcharu();

int print_backtrace(int layer);

void* better_malloc(int size);
void better_free(void* addr);


void sem_P(int id);
void sem_V(int id);
int sem_new(int num);
void printpa(void* va);


int read_cwd(char *path);
int change_cwd(const char *path);
int puttask(int pid);
int checktask();
int gettask();
void LibInit();
int kill(int pid);
void setstatus(int status);

int getstatus();
void shutdown();
void ps();
#endif
