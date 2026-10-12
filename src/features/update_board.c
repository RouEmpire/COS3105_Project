/*
 * ============================================================================
 * src/features/update_board.c
 * Feature: Update Board Title (with Educational POSIX Semaphore Guide)
 * ============================================================================
 * Welcome Students! 🎓
 *
 * In this module, you will learn about one of the most famous synchronization
 * primitives in Operating Systems: the POSIX SEMAPHORE.
 *
 * ----------------------------------------------------------------------------
 * 1. THE PROBLEM: WHY DO WE NEED SYNCHRONIZATION HERE?
 * ----------------------------------------------------------------------------
 * Our server accepts multiple client connections concurrently.
 * When a client connects, the server calls fork() (see server.c line 167) to
 * spawn a dedicated child process for that client.
 *
 * All child processes share the same board dataset in shared memory (mmap)
 * and they all persist data by writing to the same disk file ("board.txt").
 *
 * Imagine Process A and Process B both try to update a board at the exact same
 * microsecond:
 *   - Process A starts writing its version of "board.txt".
 *   - Halfway through, the CPU switches context to Process B.
 *   - Process B overwrites parts of "board.txt" with its own data.
 *   - Result: Corrupted file, interleaved lines, or lost updates!
 * This disaster is called a "RACE CONDITION".
 *
 * ----------------------------------------------------------------------------
 * 2. WHY NOT USE pthread_mutex_t?
 * ----------------------------------------------------------------------------
 * You might ask: "We learned pthread_mutex_lock in multithreading, why not here?"
 *
 * A standard pthread_mutex_t resides in the private virtual address space of a
 * single process. It coordinates threads inside the SAME process.
 * But here, we have MULTIPLE INDEPENDENT PROCESSES created by fork().
 * Process A cannot see a mutex variable sitting in Process B's memory.
 *
 * ----------------------------------------------------------------------------
 * 3. ENTER POSIX SEMAPHORES (sem_t)
 * ----------------------------------------------------------------------------
 * A POSIX Named Semaphore is created with sem_open("/store_lock", ...).
 * It is managed by the OS kernel and given a system-wide name.
 * Any process on the operating system that opens "/store_lock" accesses the
 * exact same shared kernel semaphore!
 *
 * How does a Semaphore work?
 * Think of it as a bouncer at a club holding a counter of available passes:
 *
 * 1) INITIAL STATE:
 *    - In server.c: sem = sem_open("/store_lock", O_CREAT, 0644, 1);
 *    - The counter is set to 1.
 *    - A semaphore initialized to 1 is called a "BINARY SEMAPHORE" because it
 *      only toggles between 0 (locked) and 1 (unlocked), acting like a Mutex!
 *
 * 2) sem_wait(sem)  -- "Wait" or "Lock" (Dijkstra's 'P' operation):
 *    - The process checks the counter.
 *    - If counter > 0: The counter is decremented to 0, and the process is
 *      allowed to enter the CRITICAL SECTION immediately.
 *    - If counter == 0: The counter is already 0 (another process is inside).
 *      The operating system suspends this process and puts it to sleep in a
 *      wait queue. It consumes ZERO CPU cycles while waiting!
 *
 * 3) CRITICAL SECTION:
 *    - The protected zone where shared state is modified:
 *      Updating boardTitle in memory + writing to disk file "board.txt".
 *    - Because of sem_wait, EXACTLY ONE process is guaranteed to be here.
 *
 * 4) sem_post(sem)  -- "Post" or "Unlock" (Dijkstra's 'V' operation):
 *    - When the critical section finishes, sem_post increments counter back to 1.
 *    - The OS kernel immediately wakes up any sleeping process waiting on sem_wait,
 *      allowing the next process to safely enter.
 * ============================================================================
 */

#include "update_board.h"

void handleUpdateBoard(CommandContext *ctx)
{
    char *saveptr;
    // Protocol input format: <boardId>,<newBoardTitle>
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *newBoardTitle = strtok_r(NULL, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_UPDATE_BOARD;

    // ------------------------------------------------------------------------
    // Step 1: Input Validation
    // Both Board ID and the new Title must be provided and non-empty.
    // ------------------------------------------------------------------------
    if (boardId == NULL || newBoardTitle == NULL ||
        isEmptyOrSpace(boardId) || isEmptyOrSpace(newBoardTitle))
    {
        // Status 200: STATUS_INVALID_ARGUMENTS
        printf("[UPDATE_BOARD] Invalid arguments received.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 2: Reuse Existing Parallel Board Search
    // findBoard() from get_board.h automatically inspects board count:
    //   - If < 100 boards: Fast sequential search.
    //   - If >= 100 boards: 1 thread per 100 boards parallel search.
    // ------------------------------------------------------------------------
    printf("[UPDATE_BOARD] Searching for Board ID '%s'...\n", boardId);
    Board *board = findBoard(ctx->project, boardId);

    if (board == NULL)
    {
        // Status 201: STATUS_BOARD_NOT_FOUND
        printf("[UPDATE_BOARD] Board ID '%s' not found.\n", boardId);
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // ------------------------------------------------------------------------
    // Step 3: CRITICAL SECTION GUARDED BY POSIX SEMAPHORE
    // ------------------------------------------------------------------------
    printf("[UPDATE_BOARD] Board '%s' found (Current Title: '%s').\n",
           board->boardId, board->boardTitle);

    /*
     * SEMAPHORE WAIT:
     * We decrement the semaphore counter. If another process is currently
     * saving boards or tasks, this process will pause here until the other
     * process finishes and calls sem_post.
     */
    if (sem != NULL)
    {
        printf("[UPDATE_BOARD] [SEMAPHORE] Calling sem_wait() to acquire store lock...\n");
        sem_wait(sem);
        printf("[UPDATE_BOARD] [SEMAPHORE] Lock acquired! Entering critical section.\n");
    }

    // --- CRITICAL SECTION BEGIN ---
    // 1. Update board name in shared memory
    strncpy(board->boardTitle, newBoardTitle, BOARD_TITLE_SIZE - 1);
    board->boardTitle[BOARD_TITLE_SIZE - 1] = '\0';

    // 2. Persist updated boards list to disk ("board.txt")
    int saveResult = saveBoard(ctx->project, BOARD_FILENAME);
    // --- CRITICAL SECTION END ---

    /*
     * SEMAPHORE POST:
     * We increment the semaphore counter back to 1.
     * This signals the OS kernel that the critical section is complete,
     * allowing any queued waiting processes to proceed.
     */
    if (sem != NULL)
    {
        printf("[UPDATE_BOARD] [SEMAPHORE] Calling sem_post() to release store lock...\n");
        sem_post(sem);
        printf("[UPDATE_BOARD] [SEMAPHORE] Lock released successfully.\n");
    }

    // ------------------------------------------------------------------------
    // Step 4: Transmit TCP Response Back to Client
    // ------------------------------------------------------------------------
    if (saveResult == 0)
    {
        // Status 0: STATUS_OK
        printf("[UPDATE_BOARD] Successfully updated Board ID '%s' to new title: '%s'\n",
               boardId, newBoardTitle);
        snprintf(response, sizeof(response), "%s%s%d%s%s\n",
                 cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, newBoardTitle);
    }
    else
    {
        // Status 1: STATUS_FAIL
        printf("[UPDATE_BOARD] Error saving board to file.\n");
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
