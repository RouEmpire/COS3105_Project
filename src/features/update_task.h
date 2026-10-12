#ifndef UPDATE_TASK_H
#define UPDATE_TASK_H

#include "../server.h"
#include "get_board.h"
#include "get_task.h"

/*
 * ============================================================================
 * FEATURE: UPDATE_TASK (Update Task Title)
 * ============================================================================
 * Educational Lesson for High School Students:
 *
 * 1. CODE REUSE:
 *    - To update a task, we need two identifiers: (Board ID + Task ID).
 *    - Step 1: Locate the Board via findBoard(ctx->project, boardId) from get_board.h.
 *      (Uses parallel thread search if >= 100 boards).
 *    - Step 2: Locate the Task via findTask(board, taskId) from get_task.h.
 *      (Uses parallel thread search if >= 100 tasks).
 *
 * 2. TASK ATTRIBUTES INTEGRITY:
 *    - The task update feature strictly updates the task's NAME (taskTitle).
 *    - The task's status flag (isFinish) must NOT be modified and remains intact.
 *
 * 3. PROCESS SYNCHRONIZATION WITH POSIX SEMAPHORES:
 *    - Uses POSIX Named Semaphore sem_wait(sem) and sem_post(sem) to protect
 *      the shared task database and disk persistence ("task.txt") from
 *      concurrent race conditions across forked child processes.
 * ============================================================================
 */

// Function Prototypes
void handleUpdateTask(CommandContext *ctx);

#endif // UPDATE_TASK_H
