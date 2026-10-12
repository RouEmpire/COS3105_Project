/*
 * ============================================================================
 * src/features/update_task.c
 * Feature: Update Task Title (with Deep-Dive POSIX Semaphore Guide)
 * ============================================================================
 * Educational Lesson for High School Students:
 *
 * Welcome back! In this module, we explore how POSIX SEMAPHORES guarantee
 * safe, concurrent updates to individual tasks within our Kanban board.
 *
 * ----------------------------------------------------------------------------
 * 1. THE TASK UPDATE FLOW
 * ----------------------------------------------------------------------------
 * An update operation requires three pieces of information:
 *   1) Board ID       -> Which board owns this task?
 *   2) Task ID        -> Which task on that board are we updating?
 *   3) New Task Name  -> What should the new title be?
 *
 * Notice what is NOT updated:
 *   - The task's status (isFinish) is strictly preserved!
 *     Whether a task was finished (1) or pending (0), its completion state
 *     remains untouched during a title rename.
 *
 * ----------------------------------------------------------------------------
 * 2. CODE REUSE IN ACTION
 * ----------------------------------------------------------------------------
 * Why write repetitive search code when our teammates already wrote high-performance,
 * parallel search functions?
 *
 * - findBoard(ctx->project, boardId)  [from get_board.h]:
 *   If there are >= 100 boards, this automatically divides the board array
 *   across worker threads (1 thread per 100 boards) to find the board in parallel!
 *
 * - findTask(board, taskId)           [from get_task.h]:
 *   If there are >= 100 tasks on that board, this automatically spawns worker
 *   threads (1 thread per 100 tasks) to locate the exact task concurrently!
 *
 * ----------------------------------------------------------------------------
 * 3. DEEP DIVE: POSIX SEMAPHORE MECHANICS (sem_t)
 * ----------------------------------------------------------------------------
 * When students first see semaphores, they often wonder:
 *   "How is a semaphore different from a mutex or a boolean flag like 'isLocked'?"
 *
 * Why a simple boolean variable 'int isLocked = 0;' FAILS:
 *   If Process A checks 'if (!isLocked)', and before it can set 'isLocked = 1',
 *   Process B also checks 'if (!isLocked)', BOTH processes will think they
 *   have the lock! Checking and updating a normal variable is NOT atomic.
 *
 * How POSIX Semaphores solve this:
 *   A POSIX Semaphore is maintained by the Linux kernel using hardware-level
 *   atomic CPU instructions (e.g., CMPXCHG on x86).
 *
 *   a) BINARY SEMAPHORE (Value: 0 or 1):
 *      - Value = 1: Resource is FREE (Available).
 *      - Value = 0: Resource is IN USE (Locked).
 *      This acts as a Mutual Exclusion Lock (Mutex) across processes.
 *
 *   b) COUNTING SEMAPHORE (Value: N >= 0):
 *      - Value = 5: Represents 5 available resources (e.g., 5 printer slots or
 *        5 database connections). Each sem_wait() consumes one resource until
 *        the counter hits 0.
 *
 *   c) sem_wait(sem):
 *      - Atomically decrements the counter.
 *      - If the counter drops to 0, execution continues into the Critical Section.
 *      - If already 0, the OS puts the calling process to sleep until an
 *        accompanying sem_post() arrives.
 *
 *   d) sem_post(sem):
 *      - Atomically increments the counter.
 *      - Wakes up the next waiting process in the kernel queue.
 *
 * ----------------------------------------------------------------------------
 * 4. THE CRITICAL SECTION RULES
 * ----------------------------------------------------------------------------
 * Rule 1: Keep it minimal. Only place code inside sem_wait / sem_post that
 *         actually modifies shared state (memory write + file save).
 * Rule 2: ALWAYS release the lock! If a function returns early without calling
 *         sem_post(), every other process will freeze forever (DEADLOCK).
 * ============================================================================
 */

#include "update_task.h"

void handleUpdateTask(CommandContext *ctx)
{
    char *saveptr;
    // Protocol input format: <boardId>,<taskId>,<newTaskTitle>
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskId = strtok_r(NULL, ",\n\r", &saveptr);
    char *newTaskTitle = strtok_r(NULL, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_UPDATE_TASK;

    // ------------------------------------------------------------------------
    // Step 1: Input Validation
    // We require Board ID, Task ID, and the New Task Name.
    // ------------------------------------------------------------------------
    if (boardId == NULL || taskId == NULL || newTaskTitle == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(taskId) || isEmptyOrSpace(newTaskTitle))
    {
        // Status 200: STATUS_INVALID_ARGUMENTS
        printf("[UPDATE_TASK] Invalid or missing arguments.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 2: Reusing Board Parallel Search
    // findBoard() from get_board.h (multi-threaded if total boards >= 100)
    // ------------------------------------------------------------------------
    printf("[UPDATE_TASK] Searching for Board ID '%s'...\n", boardId);
    Board *board = findBoard(ctx->project, boardId);

    if (board == NULL)
    {
        // Status 201: STATUS_BOARD_NOT_FOUND
        printf("[UPDATE_TASK] Board ID '%s' was not found.\n", boardId);
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 3: Reusing Task Parallel Search
    // findTask() from get_task.h (multi-threaded if total tasks >= 100)
    // ------------------------------------------------------------------------
    printf("[UPDATE_TASK] Searching for Task ID '%s' on Board '%s' (Total Tasks: %d)...\n",
           taskId, boardId, board->totalTasks);
    Task *task = findTask(board, taskId);

    if (task == NULL)
    {
        // Status 203: STATUS_TASK_NOT_FOUND
        printf("[UPDATE_TASK] Task ID '%s' not found on Board '%s'.\n", taskId, boardId);
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_TASK_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 4: CRITICAL SECTION GUARDED BY POSIX SEMAPHORE
    // ------------------------------------------------------------------------
    printf("[UPDATE_TASK] Found Task [ID: %s, Title: '%s', Status: %s]\n",
           task->taskId, task->taskTitle, task->isFinish ? "Finished" : "Pending");

    /*
     * SEMAPHORE WAIT:
     * Request exclusive access across all forked child processes.
     * Decrements the kernel semaphore counter from 1 to 0.
     */
    if (sem != NULL)
    {
        printf("[UPDATE_TASK] [SEMAPHORE] Acquiring store lock with sem_wait()...\n");
        sem_wait(sem);
        printf("[UPDATE_TASK] [SEMAPHORE] Lock acquired! Entering critical section.\n");
    }

    // --- CRITICAL SECTION BEGIN ---
    // 1. Update task title in shared memory
    strncpy(task->taskTitle, newTaskTitle, TASK_TITLE_SIZE - 1);
    task->taskTitle[TASK_TITLE_SIZE - 1] = '\0';

    // IMPORTANT NOTE FOR STUDENTS:
    // Notice that task->isFinish is NEVER modified here!
    // The requirement specifies updating the task's name only, keeping status intact.

    // 2. Persist all tasks to the shared file ("task.txt")
    int saveResult = saveAllTasks(ctx->project, TASK_FILENAME);
    // --- CRITICAL SECTION END ---

    /*
     * SEMAPHORE POST:
     * Release exclusive access.
     * Increments the kernel semaphore counter from 0 back to 1.
     * Wakes up any other process waiting to update boards or tasks.
     */
    if (sem != NULL)
    {
        printf("[UPDATE_TASK] [SEMAPHORE] Releasing store lock with sem_post()...\n");
        sem_post(sem);
        printf("[UPDATE_TASK] [SEMAPHORE] Lock released successfully.\n");
    }

    // ------------------------------------------------------------------------
    // Step 5: Send Response to Client Socket
    // ------------------------------------------------------------------------
    if (saveResult == 0)
    {
        // Status 0: STATUS_OK
        printf("[UPDATE_TASK] Successfully updated Task '%s' on Board '%s' to new title: '%s'!\n",
               taskId, boardId, newTaskTitle);
        snprintf(response, sizeof(response), "%s%s%d%s%s\n",
                 cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, newTaskTitle);
    }
    else
    {
        // Status 1: STATUS_FAIL
        printf("[UPDATE_TASK] Error saving tasks to file.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
