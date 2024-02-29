#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_

static inline void sync_barrier(volatile int *counter, int all) {

  int local;

  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");

  if (local + 1 < all) {
    do {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

static inline void sync_p(volatile int *counter){
  int local;
  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(-1)
               : "memory");
  if(local-1<0){
    do{
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    }while(local<0);
  }
}

static inline void sync_v(volatile int *counter){
  int local;
  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");
}

#endif