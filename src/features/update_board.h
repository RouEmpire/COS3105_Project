#ifndef UPDATE_BOARD_H
#define UPDATE_BOARD_H

#include "../server.h"
#include "get_board.h"

/*
 * ============================================================================
 * FEATURE: UPDATE_BOARD (Update Board Title)
 * ============================================================================
 * Educational Lesson for High School Students:
 *
 * 1. CODE REUSE:
 *    - To update a board, we first need to locate it.
 *    - Rather than writing a new search loop, we reuse findBoard() from
 *      get_board.h, which already implements our parallel search algorithm
 *      (1 thread per 100 boards when total boards >= 100).
 *
 * 2. PROCESS SYNCHRONIZATION WITH POSIX SEMAPHORES:
 *    - In our server architecture, every connected client is handled by a
 *      separate forked child process (via fork() in server.c).
 *    - Why can't we use standard pthread_mutex_t here?
 *      A thread mutex only works between threads sharing the same process
 *      memory address space. Child processes each have their own independent
 *      address spaces, so a mutex in Process A is completely invisible to Process B!
 *    - A POSIX Named Semaphore (sem_t) lives in kernel-backed shared memory
 *      (e.g., /dev/shm/sem.store_lock) and can synchronize completely separate
 *      operating system processes.
 *
 * 3. WHAT IS A SEMAPHORE?
 *    - Invented by Edsger Dijkstra in 1965.
 *    - A semaphore holds an integer counter that can only be adjusted by two
 *      atomic operations:
 *        * sem_wait(): Decrements counter. If counter is 0, blocks (sleeps).
 *        * sem_post(): Increments counter. Wakes up any blocked process.
 *    - When initialized with value 1, it acts as a Binary Semaphore (a Mutual
 *      Exclusion Lock or Mutex across processes).
 * ============================================================================
 */

// Function Prototypes
void handleUpdateBoard(CommandContext *ctx);

#endif // UPDATE_BOARD_H
