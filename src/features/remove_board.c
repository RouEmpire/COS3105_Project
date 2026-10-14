#include "remove_board.h"

/*
 * ============================================================================
 * COMMAND HANDLER: DELETE_BOARD (Soft Delete)
 * ============================================================================
 *
 * A soft delete deliberately keeps the board's array slot instead of shifting
 * later boards to the left.  Shifting would change the relationship between a
 * board's position and the automatically generated IDs used by addBoard().
 *
 * The deleted record is represented in memory by Board.isDeleted.  When the
 * project is saved, saveBoard() translates that flag into a line containing
 * only "---".  Thus, the on-disk file shows that the board existed but has
 * been deleted, rather than silently removing the line.
 */
void handleRemoveBoard(CommandContext *ctx)
{
    char *saveptr;
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL)
                          ? ctx->commandName
                          : COMMAND_DELETE_BOARD;

    /* A delete request needs exactly one meaningful board ID. */
    if (isEmptyOrSpace(boardId))
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /*
     * Reuse the existing finder.  It selects a sequential search for small
     * projects and a pthread-based parallel search for 100 or more boards.
     */
    Board *board = findBoard(ctx->project, boardId);
    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /*
     * Both the shared-memory change and the disk write form one critical
     * section.  The named semaphore also protects requests handled by other
     * forked server processes, not merely threads in this process.
     */
    if (sem != NULL)
        sem_wait(sem);

    board->isDeleted = 1;
    int saveResult = saveBoard(ctx->project, BOARD_FILENAME);

    if (sem != NULL)
        sem_post(sem);

    if (saveResult == 0)
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_OK);
    }
    else
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_SAVE_BOARD_FAIL);
    }

    send(ctx->clientSocket, response, strlen(response), 0);
}
