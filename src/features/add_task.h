#ifndef ADD_TASK_H
#define ADD_TASK_H

#include "../server.h"
#include "get_board.h"
#include "get_task.h"

/*
 * ============================================================================
 * FEATURE: ADD_TASK (Create Task with Parallel Board & Task Verification)
 * ============================================================================
 * Educational Parallel Programming Lesson for High School Students:
 *
 * 1. Parallel Board Lookup:
 *    Reuses findBoard() from get_board.h:
 *    - < 100 boards: Sequential search.
 *    - >= 100 boards: 1 worker thread per 100 boards.
 *
 * 2. Parallel Duplicate Task Lookup:
 *    Reuses findTaskByName() from get_task.h:
 *    - < 100 tasks: Sequential search.
 *    - >= 100 tasks: 1 worker thread per 100 tasks.
 *
 * 3. Synchronization (Semaphore & File Persistence):
 *    Acquires sem_wait(sem) to protect the shared memory array and "task.txt"
 *    before appending the new task.
 * ============================================================================
 */

void handleAddTask(CommandContext *ctx);

#endif // ADD_TASK_H
