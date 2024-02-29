/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"

#include "sync_utils.h"

volatile int exitnum = 0;
volatile int printuse=1;
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char *buf, size_t n)
{
  sync_p(&printuse);
  sprint("hartid = %d: %s", read_tp(), buf);
  
  sprint("%d %d\n",read_tp(),printuse);
  sync_v(&printuse);
  //if(read_tp()==1){sprint("ok");}
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code)
{
  if(read_tp()==1){sprint("1\n");}
  sync_p(&printuse);
  sprint("%d\n",printuse);
  sprint("hartid = %d: User exit with code:%d.\n", read_tp(), code);
  sync_v(&printuse);
  // in lab1, PKE considers only one app (one process).
  // therefore, shutdown the system when the app calls exit()
  //sprint("%d\n",exitnum);
  sync_barrier(&exitnum, NCPU);
  if(read_tp()==0){
  sprint("%d\n",exitnum);
  sprint("hartid = %d: shutdown with code:%d.\n", read_tp(), code);
  shutdown(code);
  }
  return code;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7)
{
  switch (a0)
  {
  case SYS_user_print:
    return sys_user_print((const char *)a1, a2);
  case SYS_user_exit:
    return sys_user_exit(a1);
  default:
    panic("Unknown syscall %ld \n", a0);
  }
}
