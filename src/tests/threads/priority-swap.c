/* Ensures that a high-priority thread really preempts.

   Based on priority-preempt
   Modified by Dietrich. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define LOOPS 160
#define HANG_LOOPS 10000

static void
yielder(void* arg UNUSED) {
  int i;

  msg ("Y Thread[%16s] iteration", thread_name());
  for (i=0; i<LOOPS; i++){
    msg ("Y Thread[%16s] iteration %3d", thread_name(), i);
    thread_yield();
  }
  msg("Y Thread[%16s] %30s DONE!", thread_name(), "");
}
static void
busy(void* arg UNUSED) {
  int i;

  msg ("B Thread[%16s] iteration", thread_name());
  for (i=0; i<LOOPS; i++){
    msg ("B Thread[%16s] iteration %3d", thread_name(), i);
    volatile int j;
    for (j=0; j<HANG_LOOPS; j++);
  }
  msg("B Thread[%16s] %30s DONE!", thread_name(), "");
}


void
test_priority_swap(void) {
  /* Make sure our priority is the default. */
  ASSERT(thread_get_priority() == PRI_DEFAULT);

  msg("Starting 2 Busy and 2 yielder threads with 'inverted' priorities");
  thread_create("#######", PRI_MAX-1, &busy,    NULL);
  thread_create("-", PRI_MIN+1, &yielder, NULL);
  thread_create("#", PRI_MIN+1, &yielder, NULL);
  thread_create("-------", PRI_MAX-1, &busy,    NULL);

  msg("Yield threads should have ended first!");
}

