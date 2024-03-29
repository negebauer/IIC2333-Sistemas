#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* This selects the used scheduling algorithm.
 * 'Hot Swapping' function is not required and also not expected.
 * Selection is controlled by the kernel command-line
 *  option "-o SCHEDULER" where SCHEDULER can be:
 *   - "FCFS" or "FIFO" (default)
 *   - "prioritized"
 *   - "sCFS"
 */
enum scheduling_algorithm selected_scheduler = SCH_PRIORITIZED;
next_thread_function next_thread_to_run_function;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);


/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  pick_scheduler();

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->running_times++;
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void)
{

  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);

  context_changes = 0;
  process_count = 1;
  ready_waiting_total = 0;
  average_ready_waiting = 0.0;
  global_ticks = 0;
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void)
{
  struct thread *t = thread_current ();

  global_ticks++;

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
  {
    t->quantum_run_out_times++;
    intr_yield_on_return ();
  }


}

/* Prints thread statistics. */
void
thread_print_stats (void)
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux)
{
  process_count++;
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  // ????
  enum intr_level old_level;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  // ????
  old_level = intr_disable();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  t->blocked_time = 0;
  t->blocked_times = 0;
  t->running_time = 0;
  t->running_times = 0;

  t->ready_state_time = 0;
  t->quantum_run_out_times = 0;
  t->expropied_times = 0;
  t->global_ticks_entry = 0;


  // ????
  intr_set_level(old_level);

  /* Add to run queue. */
  thread_unblock (t);
  thread_yield_to_max();

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void)
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);
  thread_current ()->blocked_times++;
  thread_current ()->status = THREAD_BLOCKED;

  schedule ();
}

/* Orders threads according to priority */
bool
 thread_priority_cmp (const struct list_elem *t1, const struct list_elem *t2, void *aux UNUSED)
{
  return list_entry (t1, struct thread, elem)->priority >
         list_entry (t2, struct thread, elem)->priority;
}

/* Return the maximal priority of all threads, or -1 if
   no other threads exist. */
static int
thread_max_priority (void)
{
  /* We need to disable interrupts here so that the race (detailed
     below) doesn't occur. */
  enum intr_level old_level;
  old_level = intr_disable ();
  int return_value = -1;

  if (list_begin (&ready_list) != list_end (&ready_list))
    {
      /* Race condition exists here if interrupts are not disabled:
         if there is only one other thread ready and that thread
         interleaves here, then blocks, then the ready list will
         be empty when we run again, even though we think it's got
         something in it, and will end up reading bogus memory below */
      struct thread *t = list_entry (list_begin (&ready_list),
                                     struct thread, elem);
      return_value = t->priority;
    }

  intr_set_level (old_level);
  return return_value;
}

/* Check to see if we're one of the highest-priority threads.
   If not, yield. */
void
thread_yield_to_max (void)
{
  if (thread_max_priority () > thread_get_priority ())
    thread_yield ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t)
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  // list_push_back (&ready_list, &t->elem);
  list_insert_ordered(&ready_list, &t->elem, thread_priority_cmp, NULL);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}



/* Returns the name of the running thread. */
const char *
thread_name (void)
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void)
{
  struct thread *t = running_thread ();

  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void)
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void)
{
  struct thread *t = thread_current();
  printf("-------------------------------------------\n");
  printf("%s finished:\n", t->name);
  printf("Times blocked: %d (%d ticks)\n", t->blocked_times, t->blocked_time);
  printf("Times running: %d (%d ticks)\n", t->running_times, t->running_time);
  printf("Time ready status: %d ticks\n", t->ready_state_time);
  printf("Quantums run out times: %d\n", t->quantum_run_out_times);
  printf("Expropied times: %d\n", t->expropied_times);
  printf("-------------------------------------------\n");

  ready_waiting_total += t->running_time;

  if (t->name == initial_thread->name)
  {
    printf("===========================================\n");
    printf("Context changes: %d\n", context_changes);
    printf("Executed processes: %d\n", process_count);
    average_ready_waiting = ready_waiting_total /  process_count;
    printf("Average ready status waiting: %d\n", average_ready_waiting);
    printf("===========================================\n");
  }

  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  cur->running_time += global_ticks - cur->global_ticks_entry;
  enum intr_level old_level;

  ASSERT (!intr_context ());

  cur->expropied_times++;
  context_changes++;

  old_level = intr_disable ();
  if (cur != idle_thread)
    // list_push_back (&ready_list, &cur->elem);
    list_insert_ordered(&ready_list, &cur->elem, thread_priority_cmp, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}


/* Return the maximum donated priority of a thread */
static int thread_get_donated_priority(struct thread *t)
{
  ASSERT(is_thread(t));

  // ????
  enum intr_level old_level = intr_disable();
  int return_value = -1;

  if (list_begin(&t->priority_donations) != list_end(&t->priority_donations))
  {
    struct thread *top = list_entry(list_begin(&t->priority_donations), struct thread, donation_elem);
    return_value = top->priority;
  }

  // ????
  intr_set_level(old_level);
  return return_value;
}

/* Removes a thread, and re-inserts it back, so te ready list maintains ordered */
static void thread_reinsert_ready_list(struct thread *t)
{
  if (t->status == THREAD_READY)
  {
    ASSERT(intr_get_level() == INTR_OFF);
    list_remove(&t->elem);
    list_insert_ordered(&ready_list, &t->elem, thread_priority_cmp, NULL);
  }
}

/* Calculates and sets the current thread priority (with donations) */
void thread_calculate_priority(struct thread *t)
{
  ASSERT(is_thread(t));

  // ????
  enum intr_level old_level = intr_disable();

  int donated_priority = thread_get_donated_priority(t);
  if (donated_priority > t->base_priority)
  {
    t->priority = donated_priority;
  }
  else
  {
    t->priority = t->base_priority;
  }

  thread_reinsert_ready_list(t);
  intr_set_level(old_level);
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority)
{
  thread_current ()->base_priority = new_priority;
  thread_calculate_priority(thread_current());
  thread_yield_to_max ();
}

/* Returns the current thread's priority. */
int
thread_get_priority (void)
{
  return thread_current ()->priority;
}

/* Donation compare function for list donation */
static bool
thread_donation_cmp (const struct list_elem *a,
                     const struct list_elem *b,
                     void *aux UNUSED)
{
  return list_entry (a, struct thread, donation_elem)->priority >
         list_entry (b, struct thread, donation_elem)->priority;
}

/* Recompute the priority of thread, and donate if necesary. recurssion */
void thread_donate_priority (struct thread *t)
{
  while (true)
  {
    ASSERT(intr_get_level() == INTR_OFF);
    ASSERT(is_thread(t));

    /* Recalculate our priority */
    thread_calculate_priority(t);

    if (t->waiting_on != NULL)
    {
      struct thread *waiter = t->waiting_on->holder;
      ASSERT(waiter != t);

      if (thread_current() != t)
        thread_recall_donation(t);

      if (waiter != NULL)
      {
        thread_calculate_priority(waiter);
        ASSERT(is_thread(waiter));

        /* Make donation */
        list_insert_ordered(&waiter->priority_donations, &t->donation_elem, thread_donation_cmp, NULL);

        t = waiter;
      }
      else
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
}

/* Remove the given thread donation, assumes that it has already made a donation, and the donor doesn't need to recompute effective priority */
void thread_recall_donation (struct thread *t)
{
    ASSERT(intr_get_level() == INTR_OFF);
    ASSERT(is_thread(t));

    /* Make sure to have made a donation */
    if (t->donation_elem.next != NULL)
    {
      list_remove(&t->donation_elem);
      t->donation_elem.next = NULL;
    }

}



/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED)
{
  /* Not yet implemented. */
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void)
{
  /* Not yet implemented. */
  return 0;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void)
{
  /* Not yet implemented. */
  return 0;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void)
{
  /* Not yet implemented. */
  return 0;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED)
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;)
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux)
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void)
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  // ????
  //enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  t->blocked_times++;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->base_priority = priority;
  t->magic = THREAD_MAGIC;
  // ????
  //old_level = intr_disable ();
  list_init (&t->priority_donations);
  list_push_back (&all_list, &t->allelem);
  //list_insert_ordered(&all_list, &t->allelem, &cmp_thread_priority, NULL);

  // ????
  //intr_set_level (old_level);

}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size)
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}


/** First Come, First Serve. */
static struct thread *
next_thread_to_run_fcfs (void)
{
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** Multi-level feedback queue scheduler */
static struct thread *
next_thread_to_run_mlfqs (void)
{
  //unimplemented, just for completeness
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

bool most_pri (const struct list_elem *a,
               const struct list_elem *b,
               void *aux) {
  struct thread *ta = list_entry (a, struct thread, elem);
  struct thread *tb = list_entry (b, struct thread, elem);
  ASSERT (is_thread (ta));
  ASSERT (is_thread (tb));
  return ta->priority > tb->priority;
}

/** Prioritized Scheduling
 *
 * Should avoid deadlocks when a priority process waits for another with
 *  lower priority.
 */
static struct thread *
next_thread_to_run_prioritized (void)
{
  list_sort (&ready_list, &most_pri, NULL);
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** Simplified implementation of the Completely Fair Scheduler */
static struct thread *
next_thread_to_run_sCFS (void)
{
  //OLDTODO: Implement Simple Completely Fair Scheduler
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** Lottery Scheduler */
static struct thread *
next_thread_to_run_lottery (void)
{
  //OLDTODO: Implement Prioritized Lottery Scheduler
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** Dynamic Lottery Scheduler */
static struct thread *
next_thread_to_run_dyn_Lottery (void)
{
  //OLDTODO: Implement Dynamic Priority Lottery Scheduler
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** Dual Queue Scheduler */
static struct thread *
next_thread_to_run_dq (void)
{
  //FIXME: Actually it performs FCFS (FIFO).
  //TODO: Implement Dual Queue Scheduler
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/** N Queues Scheduler */
static struct thread *
next_thread_to_run_nq (void)
{
  //FIXME: Actually it performs FCFS (FIFO).
  //TODO: Implement N Queues Scheduler
  return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

void
pick_scheduler(void)
{
  switch(selected_scheduler){
    case SCH_MLFQS:
      printf("Using MLFQS as the CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_mlfqs;
      break;
    case SCH_PRIORITIZED:
      printf("Using a prioritized CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_prioritized;
      break;
    case SCH_SIMPLE_CFS:
      printf("Using a simple implementation of the CFS as the CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_sCFS;
      break;
    case SCH_LOTTERY:
      printf("Using prioritized Lottery as the CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_lottery;
      break;
    case SCH_DYN_LOTTERY:
      printf("Using a dynamic prioritized Lottery as the CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_dyn_Lottery;
      break;
    case SCH_DQ:
      printf("Using a dual queue CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_dq;
      break;
    case SCH_N_QUEUES:
      printf("Using a %d-queues CPU Scheduler\n", queueCount);
      next_thread_to_run_function = next_thread_to_run_nq;
      break;
    case SCH_FCFS:
    default:
      printf("Using FCFS as the CPU Scheduler\n");
      next_thread_to_run_function = next_thread_to_run_fcfs;
      break;
  }
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void)
{
  //NOTE: This default behaviour should be moved into the next thread to run
  //        functions if not all of them need it (they may not use ready_list).
  if (list_empty (&ready_list))
    return idle_thread;

  return next_thread_to_run_function();
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  cur->running_times++;

  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;
  cur->global_ticks_entry = global_ticks;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void)
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);

  //check_priority();
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void)
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
