#include "add_board.h"

/*
 * ============================================================================
 * COMMAND HANDLER: ADD_BOARD (Create Board)
 * ============================================================================
 * Educational Explanation for High School CS Students:
 * 1. Sanitizes user input (extracts the new board title).
 * 2. Uses a Named Semaphore (sem_wait) to acquire a mutual exclusion lock
 *    across multiple forked server processes.
 * 3. Appends the new board to shared memory (addBoard).
 * 4. Persists the updated board list to disk ("board.txt") via saveBoard.
 * 5. Releases the semaphore lock (sem_post) so other processes can access.
 * 6. Returns response status (STATUS_OK = 0) back to the client over TCP.
 */
void handleAddBoard(CommandContext *ctx)
{
    char *saveptr;
    char *boardTitle = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_ADD_BOARD;

    // Validation: Ensure board title is provided and not empty
    if (isEmptyOrSpace(boardTitle))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // --- CRITICAL SECTION BEGIN ---
    // 1. Acquire semaphore lock (block until available)
    sem_wait(sem);

    // 2. Add board to project in shared memory
    int result = addBoard(ctx->project, boardTitle);

    // 3. Save updated boards to disk file ("board.txt")
    saveBoard(ctx->project, BOARD_FILENAME);

    // 4. Release semaphore lock
    sem_post(sem);
    // --- CRITICAL SECTION END ---

    if (result == 0)
    {
        // Status 0: STATUS_OK
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_OK);
    }
    else
    {
        // Status 1: STATUS_FAIL
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
