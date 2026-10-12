/*
 * ============================================================================
 * src/features/add_task.c
 * Feature: Add Task (with Parallel Board and Task Search)
 * ============================================================================
 * Educational Lesson for High School Students:
 *
 * When creating a new task, we demonstrate how to REUSE existing parallel
 * algorithms rather than reinventing the wheel!
 *
 * 1. REUSING BOARD PARALLEL SEARCH (findBoard from get_board.h):
 *    - If total boards < 100: uses a fast sequential loop.
 *    - If total boards >= 100: launches worker threads (1 thread per 100 boards)
 *      to locate the target board concurrently.
 *
 * 2. REUSING TASK PARALLEL SEARCH (findTaskByName from get_task.h):
 *    - Before adding, we verify whether a task with the same title already exists.
 *    - If total tasks < 100: uses a fast sequential loop.
 *    - If total tasks >= 100: launches worker threads (1 thread per 100 tasks)
 *      to search through the board's task list in parallel.
 *
 * 3. PROCESS SYNCHRONIZATION (POSIX Semaphore sem_wait / sem_post):
 *    - Because our server uses forked child processes, we use a POSIX Semaphore
 *      lock across processes so two users don't write to "task.txt" simultaneously.
 * ============================================================================
 */

#include "add_task.h"

void handleAddTask(CommandContext *ctx)
{
    char *saveptr;
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskTitle = strtok_r(NULL, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_ADD_TASK;

    // Validation: Both Board ID and Task Name are required
    if (boardId == NULL || taskTitle == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(taskTitle))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 1: Parallel Board Lookup
    // Reuses findBoard() from get_board.h:
    // Uses 1 thread per 100 boards if >= 100 boards.
    // ------------------------------------------------------------------------
    printf("[ADD_TASK] Finding Board '%s' using parallel search...\n", boardId);
    Board *board = findBoard(ctx->project, boardId);

    if (board == NULL)
    {
        // Status 201: Board Not Found
        printf("[ADD_TASK] Board ID '%s' was not found.\n", boardId);
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 2: Parallel Task Lookup (Duplicate Check)
    // Reuses findTaskByName() from get_task.h:
    // Uses 1 thread per 100 tasks if >= 100 tasks.
    // ------------------------------------------------------------------------
    printf("[ADD_TASK] Checking if task '%s' already exists on board '%s' (Total Tasks: %d)...\n",
           taskTitle, boardId, board->totalTasks);
    Task *existingTask = findTaskByName(board, taskTitle);

    if (existingTask != NULL)
    {
        // Duplicate task found on this board
        printf("[ADD_TASK] Duplicate task '%s' found with ID '%s'.\n",
               taskTitle, existingTask->taskId);
        snprintf(response, sizeof(response), "%s%s%d%sTask already exists%s%s\n",
                 cmd, COMMAND_SEPARATOR,
                 STATUS_INVALID_ARGUMENTS, COMMAND_SEPARATOR,
                 COMMAND_SEPARATOR, existingTask->taskId);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 3: CRITICAL SECTION (Semaphore Lock & File Persistence)
    // ------------------------------------------------------------------------
    if (sem != NULL) {
        sem_wait(sem); // Acquire lock across processes
    }

    int result = addTask(board, taskTitle);
    saveAllTasks(ctx->project, TASK_FILENAME);

    if (sem != NULL) {
        sem_post(sem); // Release lock
    }

    // ------------------------------------------------------------------------
    // Step 4: Send Response
    // ------------------------------------------------------------------------
    if (result == 0)
    {
        char *newTaskId = board->tasks[board->totalTasks - 1].taskId;
        printf("[ADD_TASK] Successfully created task '%s' [ID: %s] on Board '%s'!\n",
               taskTitle, newTaskId, boardId);
        snprintf(response, sizeof(response), "%s%s%d%s%s\n",
                 cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, newTaskId);
    }
    else
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
