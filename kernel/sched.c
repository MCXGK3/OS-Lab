/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "sync_utils.h"

process* ready_queue_head[NCPU] = {NULL};
int shutdown_barrier=0;
//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue( process* proc ) {
  sprint( "going to insert process %d to ready queue.\n", proc->pid );
  // if the queue is empty in the beginning
  if( ready_queue_head[read_tp()] == NULL ){
    proc->status = READY;
    proc->queue_next = NULL;
    ready_queue_head[read_tp()] = proc;
    return;
  }

  // ready queue is not empty
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p=ready_queue_head[read_tp()]; p->queue_next!=NULL; p=p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p==proc ) return;
  p->queue_next = proc;
  proc->status = READY;
  proc->queue_next = NULL;

  return;
}

//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the current
// process is still runnable, you should place it into the ready queue (by calling
// ready_queue_insert), and then call schedule().
//
extern process procs[NPROC];
void schedule() {
  if ( !ready_queue_head [read_tp()]){
    // printerr("hartid%d shutdown\n",read_tp());
    sync_barrier(&shutdown_barrier,NCPU);
    // by default, if there are no ready process, and all processes are in the status of
    // FREE and ZOMBIE, we should shutdown the emulated RISC-V machine.
    int should_shutdown = 1;

    for( int i=0; i<NPROC; i++ )
      if( (procs[i].status != FREE) && (procs[i].status != ZOMBIE) ){
        should_shutdown = 0;
        sprint("proc status is in %p\n",&procs[i].status);
        sprint( "ready queue empty, but process %d is not in free/zombie state:%d\n", 
          i, procs[i].status );
      }

    if( should_shutdown ){
      if(read_tp()==0){
      sprint( "no more ready processes, system shutdown now.\n" );
      shutdown( 0 );
      }
      else return;
    }else{
      panic( "Not handled: we should let system wait for unfinished processes.\n" );
    }
  }

  current[read_tp()] = ready_queue_head[read_tp()];
  assert( current[read_tp()]->status == READY );
  ready_queue_head[read_tp()] = ready_queue_head[read_tp()]->queue_next;

  current[read_tp()]->status = RUNNING;
  sprint( "hartid=%d going to schedule process %d to run.\n", read_tp(),current[read_tp()]->pid );
  switch_to( current[read_tp()] );
}

int do_wait(int id){
  if(id<-1||id==0) return -1;
  else if(id==-1){
    process *parent=current[read_tp()];
    process *p;
    for(p=ready_queue_head[read_tp()];p!=NULL;p=p->queue_next){
        if(p->parent==parent){
          parent->status=BLOCKED;
          p->wait=1;
          schedule();
          return p->pid;
        }
    }
    return -1;
  }
  else{
    process *parent=current[read_tp()];
    process *p;
    for(p=ready_queue_head[read_tp()];p!=NULL;p=p->queue_next){
      if(p->pid==id){
        if(p->parent==parent){
          parent->status=BLOCKED;
          p->wait=1;
          schedule();
          return p->pid;
        }
        else{return -1;}
      }
    } 
  return-1;
  }
}
