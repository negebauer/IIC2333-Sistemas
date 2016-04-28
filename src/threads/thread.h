#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Available scheduling algorithms */
enum scheduling_algorithm
  {
    SCH_FCFS,           /* First Come, First Serve. Such FIFO, much wow. */
    SCH_MLFQS,          /* PintOS default scheduling assignment. */
    SCH_PRIORITIZED,    /* 2014/1, Hw1, Pt1 */
    SCH_SIMPLE_CFS,     /* 2014/1, Hw1, Pt2 */
    SCH_LOTTERY,        /* 2014/2, Hw1, Pt1 */
    SCH_DYN_LOTTERY,    /* 2014/2, Hw1, Pt2 */
    SCH_DQ,             /* 2015/1, Hw1, Pt1 */
    SCH_N_QUEUES        /* 2015/2, Hw1, Pt1 */
  };


/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
typedef struct thread *(*next_thread_function)(void);

int queueCount;

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */

    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* Priority donation */
    int base_priority;                  /* Initial Priority (without donations) */
    int priority;                       /* Priority including donations */
    struct lock* waiting_on;            /* lock that thread is waiting to acquire */
    struct list priority_donations;     /* Sorted-List for priority donations */
    struct list_elem donation_elem;     /* Actual donation element */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */

    /* Local statistics*/
    int16_t blocked_time;
    int blocked_times;
    int16_t running_time;
    int running_times;

    int16_t ready_state_time;
    int quantum_run_out_times;
    int expropied_times;


  };

/* Global statistics */
int context_changes;
int process_count;
int ready_waiting_total;
float average_ready_waiting;


/* This selects the used scheduling algorithm.
 * 'Hot Swapping' function is not required and also not expected.
 * Selection is controlled by the kernel command-line
 *  option "-o SCHEDULER" where SCHEDULER can be:
 *   - "FCFS" or "FIFO" (default)
 *   - "prioritized"
 *   - "sCFS"
 */
extern enum scheduling_algorithm selected_scheduler;
void pick_scheduler(void);

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);
void thread_yield_to_max(void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

void thread_calculate_priority(struct thread *t);
void thread_donate_priority(struct thread *t);
bool thread_priority_cmp (const struct list_elem *t1, const struct list_elem *t2, void *unused UNUSED);

int thread_get_priority (void);
void thread_set_priority (int);
void thread_recall_donation(struct thread *t);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);



#endif /* threads/thread.h */
