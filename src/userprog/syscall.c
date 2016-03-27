#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"


// Implementation Parameters
// =========================
static int WRITE_SIZE=128;



inline
int
syscall_write(struct intr_frame *f)
{
  int *p = f->esp;  // Stack pointer (x86 registers http://www.scs.stanford.edu/05au-cs240c/lab/i386/s02_03.htm)

  // FIXME: This is (probably) displaced because of the -12 on process.c:442 (setup_stack)
  int sp_offset = 0;  // after removing the -=12, decreasing this should fix this

  int file_descriptor =         p[sp_offset+1];
  char        *buffer = (char*) p[sp_offset+2];
  int            size = (int)   p[sp_offset+3];

  switch(file_descriptor)
  {
    case STDIN_FILENO:
      // Inform that no data was written
      f->eax=0;
      return 2;

    case STDOUT_FILENO:
    {
      // Write in chunks of WRITE_SIZE
      //   putbuf (src/lib/kernel/console.c) locks the console,
      //   we should avoid locking it too often or for too long
      int remaining = size;
      while(remaining > WRITE_SIZE)
      {
        // Write a chunk
        putbuf(buffer, WRITE_SIZE);
        // Advance buffer pointer
        buffer    += WRITE_SIZE;
        remaining -= WRITE_SIZE;
      }
      // Write all the remaining data
      putbuf(buffer, remaining);

      // Inform the amount of data written
      f->eax=(int)size;
      return 0;
    }

    default:
      printf("syscall: write call not implemented for files\n");
      return 1;
  }

  return 1;  // Unreachable, but compiler complains
}



static void
syscall_handler (struct intr_frame *f)
{
  // intr_frame holds CPU register data
  //   Intel 80386 Reference Programmer's Manual (TL;DR)
  //     http://www.scs.stanford.edu/05au-cs240c/lab/i386/toc.htm

  int *p = f->esp;  // Stack pointer (x86 registers http://www.scs.stanford.edu/05au-cs240c/lab/i386/s02_03.htm)
  int syscall_number = (*p);

  switch(syscall_number)
  {
    case SYS_HALT:
      printf("system call: halt\n");
      break;

    case SYS_EXIT:
      printf("system call: exit\n");
      break;

    case SYS_EXEC:
      printf("system call: exec\n");
      break;

    case SYS_WAIT:
      printf("system call: wait\n");
      break;

    case SYS_WRITE:
      syscall_write(f);
      return;

    default:
      printf("system call: unhandled syscall. Terminating process[%d]\n",
             thread_current()->tid);
      break;
  }

  // Syscall handling failed, terminate the process
  thread_exit ();
}


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, &syscall_handler, "syscall");
}
