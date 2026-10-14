/*
 * ============================================================================
 * src/features/set_task_status.c
 * Feature: Toggle Task Completion Status (Done ⇄ Pending)
 * ============================================================================
 * Educational Lesson for High School Students — Parallel Programming Class:
 *
 * This file shows how a simple "toggle" operation becomes non-trivial when
 * multiple processes share the same data.  Follow along step-by-step!
 *
 * ── OVERVIEW ────────────────────────────────────────────────────────────────
 *
 *   Client sends:   SET_TASK_STATUS,<boardId>,<taskId>
 *   Server does:
 *     1. Validate input  (are boardId and taskId present?)
 *     2. Find the board  (reuse findBoard — parallel if >= 100 boards)
 *     3. Find the task   (reuse findTask  — parallel if >= 100 tasks)
 *     4. CRITICAL SECTION (protected by POSIX Semaphore):
 *        a. Toggle:  task->isFinish = !task->isFinish
 *        b. Save:    saveAllTasks(project, "task.txt")
 *     5. Send response back to client
 *
 * ── WHY THE SEMAPHORE MATTERS HERE ──────────────────────────────────────────
 *
 * Imagine two users toggle the same task at the same time:
 *
 *   Without Semaphore:
 *     Process A reads  isFinish = 0
 *     Process B reads  isFinish = 0       (before A writes)
 *     Process A writes isFinish = 1       (toggle: 0 → 1)
 *     Process B writes isFinish = 1       (toggle: 0 → 1, WRONG! should be 1 → 0)
 *     ⇒ Both processes see "Pending" and both set it to "Done".
 *        The second toggle is LOST.  This is the classic "Lost Update" bug.
 *
 *   With Semaphore:
 *     Process A:  sem_wait()  → enters critical section
 *     Process B:  sem_wait()  → BLOCKED (sleeps until A releases)
 *     Process A:  reads 0, writes 1, saves, sem_post()
 *     Process B:  wakes up, reads 1, writes 0, saves, sem_post()
 *     ⇒ Both toggles execute correctly in sequence.  No lost updates!
 *
 * ── THE TOGGLE AS A READ-MODIFY-WRITE ───────────────────────────────────────
 *
 * Even a single line like  task->isFinish = !task->isFinish  is actually
 * THREE CPU operations at the machine code level:
 *
 *   LOAD   R1, [task->isFinish]     ← Read current value into a register
 *   XOR    R1, 1                    ← Flip the bit (0↔1)
 *   STORE  [task->isFinish], R1     ← Write the new value back
 *
 * The OS can context-switch between ANY of those instructions, so the
 * semaphore must wrap ALL of them (plus the file save) to be safe.
 * ============================================================================
 */

#include "set_task_status.h"

void handleSetTaskStatus(CommandContext *ctx)
{
    char *saveptr;
    /*
     * Protocol input format:  <boardId>,<taskId>
     * (No third argument — we simply flip the current status.)
     */
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskId  = strtok_r(NULL,          ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_SET_TASK_STATUS;

    /* ────────────────────────────────────────────────────────────────────────
     * Step 1: Input Validation
     * Both Board ID and Task ID are mandatory.
     * ──────────────────────────────────────────────────────────────────────── */
    if (boardId == NULL || taskId == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(taskId))
    {
        printf("[SET_TASK_STATUS] Invalid or missing arguments.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /* ────────────────────────────────────────────────────────────────────────
     * Step 2: Reuse Parallel Board Search
     * findBoard() from get_board.h:
     *   - < 100 boards  → sequential loop   (fast for small datasets)
     *   - >= 100 boards → spawns 1 pthread per 100 items (parallel search)
     * ──────────────────────────────────────────────────────────────────────── */
    printf("[SET_TASK_STATUS] Searching for Board ID '%s'...\n", boardId);
    Board *board = findBoard(ctx->project, boardId);

    if (board == NULL)
    {
        printf("[SET_TASK_STATUS] Board ID '%s' was not found.\n", boardId);
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /* ────────────────────────────────────────────────────────────────────────
     * Step 3: Reuse Parallel Task Search
     * findTask() from get_task.h:
     *   - < 100 tasks  → sequential loop
     *   - >= 100 tasks → spawns 1 pthread per 100 items (parallel search)
     * ──────────────────────────────────────────────────────────────────────── */
    printf("[SET_TASK_STATUS] Searching for Task ID '%s' on Board '%s' (Total Tasks: %d)...\n",
           taskId, boardId, board->totalTasks);
    Task *task = findTask(board, taskId);

    if (task == NULL)
    {
        printf("[SET_TASK_STATUS] Task ID '%s' not found on Board '%s'.\n", taskId, boardId);
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_TASK_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /* ────────────────────────────────────────────────────────────────────────
     * Step 4: CRITICAL SECTION — Toggle Status under Semaphore Protection
     * ────────────────────────────────────────────────────────────────────────
     * Remember: our server uses fork() so each client runs in a separate
     * operating system process.  A POSIX Named Semaphore is the correct
     * synchronization primitive because it works ACROSS processes (unlike
     * pthread_mutex_t which only works across threads within ONE process).
     */
    int oldStatus = task->isFinish;
    printf("[SET_TASK_STATUS] Task '%s' current status: %s (%d)\n",
           task->taskTitle, oldStatus ? "Done" : "Pending", oldStatus);

    /*
     * sem_wait(sem)  — Dijkstra's P (proberen / "try") operation:
     *   Atomically decrements the semaphore counter.
     *   If the counter was already 0, the OS suspends this process until
     *   another process calls sem_post().
     */
    if (sem != NULL)
    {
        printf("[SET_TASK_STATUS] [SEMAPHORE] Acquiring lock with sem_wait()...\n");
        sem_wait(sem);
        printf("[SET_TASK_STATUS] [SEMAPHORE] Lock acquired! Entering critical section.\n");
    }

    /* ═══════════════════ CRITICAL SECTION BEGIN ═══════════════════
     *
     * TOGGLE: Flip the isFinish flag between 0 and 1.
     *
     * In C, the logical NOT operator (!) converts:
     *   0  →  1   (Pending  →  Done)
     *   1  →  0   (Done     →  Pending)
     *
     * This is a READ-MODIFY-WRITE operation:
     *   READ:   load task->isFinish
     *   MODIFY: compute !task->isFinish
     *   WRITE:  store result back into task->isFinish
     *
     * The semaphore ensures no other process can interleave between
     * these three steps.
     */
    task->isFinish = !task->isFinish;
    int newStatus = task->isFinish;

    /* Persist the updated task list to "task.txt" on disk. */
    int saveResult = saveAllTasks(ctx->project, TASK_FILENAME);

    /* ═══════════════════ CRITICAL SECTION END ═══════════════════ */

    /*
     * sem_post(sem)  — Dijkstra's V (verhogen / "increment") operation:
     *   Atomically increments the semaphore counter back to 1.
     *   Wakes up the next process sleeping in sem_wait().
     */
    if (sem != NULL)
    {
        printf("[SET_TASK_STATUS] [SEMAPHORE] Releasing lock with sem_post()...\n");
        sem_post(sem);
        printf("[SET_TASK_STATUS] [SEMAPHORE] Lock released.\n");
    }

    /* ────────────────────────────────────────────────────────────────────────
     * Step 5: Send Response to the Client
     * ──────────────────────────────────────────────────────────────────────── */
    if (saveResult == 0)
    {
        printf("[SET_TASK_STATUS] Toggled Task '%s': %s → %s\n",
               task->taskTitle,
               oldStatus ? "Done" : "Pending",
               newStatus ? "Done" : "Pending");

        /*
         * Response format:  SET_TASK_STATUS,0,<taskTitle>,<newStatus>
         *   newStatus = 0 means Pending
         *   newStatus = 1 means Done
         */
        snprintf(response, sizeof(response), "%s%s%d%s%s%s%d\n",
                 cmd, COMMAND_SEPARATOR,
                 STATUS_OK, COMMAND_SEPARATOR,
                 task->taskTitle, COMMAND_SEPARATOR,
                 newStatus);
    }
    else
    {
        printf("[SET_TASK_STATUS] Error: Failed to save tasks to disk.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
