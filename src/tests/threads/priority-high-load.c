/* Ensures that a high-load thread really preempts.

   Based on priority-preempt
   Modified by Dietrich. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define LOOPS 10000
#define HANG_LOOPS 20000000

static void
really_busy(void* arg UNUSED) {
  int i;

  msg ("B(%3d) Thread[%16s] iteration", thread_get_priority(), thread_name());
  for (i=0; i<=LOOPS; i++)
    if(i%(LOOPS/10)==0){
      msg ("B(%3d) Thread[%16s] iteration %3d", thread_get_priority(), thread_name(), i);
      volatile int j;
      for (j=0; j<HANG_LOOPS; j++);
    }
  msg("B(%3d) Thread[%16s] %30s DONE!", thread_get_priority(), thread_name(), "");
}


void
test_priority_high_load(void) {
  /* Make sure our priority is the default. */
  ASSERT(thread_get_priority() == PRI_DEFAULT);

  msg("Starting 5 Busy threads");
  thread_create("#######", PRI_MAX-1,     &really_busy, NULL);
  thread_create("-------", PRI_MAX-5,     &really_busy, NULL);
  thread_create("___",     PRI_DEFAULT-1, &really_busy, NULL);
  thread_create("+++",     PRI_DEFAULT+1, &really_busy, NULL);
  thread_create("---",     PRI_DEFAULT,   &really_busy, NULL);

  msg("No thread should have held the CPU too long!");
}

