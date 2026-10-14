#include "remove_task.h"

/*
 * DELETE_TASK performs a soft delete: the task array position stays allocated
 * so later task IDs do not change.  saveAllTasks() persists that deleted slot
 * as "<boardId>,---", retaining only the information needed to associate the
 * tombstone with its board when task.txt is loaded again.
 */
void handleRemoveTask(CommandContext *ctx)
{
    char *saveptr;
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskId = strtok_r(NULL, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL)
                          ? ctx->commandName
                          : COMMAND_DELETE_TASK;

    /* The protocol requires both identifiers. */
    if (isEmptyOrSpace(boardId) || isEmptyOrSpace(taskId))
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /* Reuse the existing adaptive sequential/parallel search functions. */
    Board *board = findBoard(ctx->project, boardId);
    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    Task *task = findTask(board, taskId);
    if (task == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n",
                 cmd, COMMAND_SEPARATOR, STATUS_TASK_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    /* Protect the shared-memory flag update and file rewrite as one operation. */
    if (sem != NULL)
        sem_wait(sem);

    task->isDeleted = 1;
    int saveResult = saveAllTasks(ctx->project, TASK_FILENAME);

    if (sem != NULL)
        sem_post(sem);

    snprintf(response, sizeof(response), "%s%s%d\n",
             cmd, COMMAND_SEPARATOR,
             saveResult == 0 ? STATUS_OK : STATUS_SAVE_TASK_FAIL);
    send(ctx->clientSocket, response, strlen(response), 0);
}
