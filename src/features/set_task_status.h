#ifndef SET_TASK_STATUS_H
#define SET_TASK_STATUS_H

#include "../server.h"
#include "get_board.h"
#include "get_task.h"

/*
 * ============================================================================
 * FEATURE: SET_TASK_STATUS (Toggle Task Completion Status)
 * ============================================================================
 * Educational Lesson for High School Students — Parallel Programming Class:
 *
 * This feature toggles a task's "isFinish" flag:
 *   - If the task is currently Pending   (isFinish == 0)  →  set to Done     (1)
 *   - If the task is currently Done      (isFinish == 1)  →  set to Pending  (0)
 *
 * HOW THIS MODULE DEMONSTRATES KEY PARALLEL PROGRAMMING CONCEPTS:
 *
 * 1. CODE REUSE OF PARALLEL SEARCH FUNCTIONS:
 *    We reuse findBoard() and findTask() — both of which internally launch
 *    POSIX threads (pthreads) when the dataset is large (>= 100 items).
 *    This means our simple "set status" feature automatically benefits from
 *    multi-threaded parallel search without writing any thread code here!
 *
 * 2. PROCESS-LEVEL SYNCHRONIZATION WITH POSIX SEMAPHORES:
 *    Our server forks a new child process for every client connection.
 *    Two clients could try to toggle the same task at the same microsecond.
 *    We use a Named POSIX Semaphore (sem_wait / sem_post) to ensure only
 *    ONE process modifies the shared task data at a time.
 *
 * 3. ATOMIC TOGGLE PATTERN:
 *    The toggle operation  task->isFinish = !task->isFinish  looks simple,
 *    but it is actually a READ-MODIFY-WRITE sequence:
 *      Step A: READ  the current value (e.g., 0)
 *      Step B: MODIFY it              (compute !0 = 1)
 *      Step C: WRITE the new value    (store 1)
 *    Without the semaphore, Process A could READ 0, get preempted, Process B
 *    also READS 0, both WRITE 1, and the toggle only happened once instead
 *    of twice!  The semaphore prevents this interleaving.
 *
 * PROTOCOL:
 *   Command:  SET_TASK_STATUS,<boardId>,<taskId>
 *   Success:  SET_TASK_STATUS,0,<taskTitle>,<newStatus>
 *                where newStatus is 0 (Pending) or 1 (Done)
 *   Errors:   SET_TASK_STATUS,200   (invalid arguments)
 *             SET_TASK_STATUS,201   (board not found)
 *             SET_TASK_STATUS,203   (task not found)
 *             SET_TASK_STATUS,1     (save failure)
 * ============================================================================
 */

// Function Prototypes
void handleSetTaskStatus(CommandContext *ctx);

#endif // SET_TASK_STATUS_H
