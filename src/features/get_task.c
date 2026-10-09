#include "get_task.h"

void handleGetTask(CommandContext *ctx)
{
    char *saveptr;
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskId = strtok_r(NULL, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_GET_TASK;

    if (isEmptyOrSpace(boardId))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    Board *board = hasBoard(ctx->project, boardId);
    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Case 1: Specific Task ID requested (GET_TASK)
    if (!isEmptyOrSpace(taskId))
    {
        Task *task = hasTask(ctx->project, boardId, taskId);
        if (task == NULL)
        {
            snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_TASK_NOT_FOUND);
            send(ctx->clientSocket, response, strlen(response), 0);
            return;
        }

        snprintf(response, sizeof(response), "%s%s%d%s%s%s%s%s%d\n",
                 cmd, COMMAND_SEPARATOR,
                 STATUS_OK, COMMAND_SEPARATOR,
                 task->taskId, COMMAND_SEPARATOR,
                 task->taskTitle, COMMAND_SEPARATOR,
                 task->isFinish);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Case 2: List all tasks for board (GET_TASKS)
    if (board->totalTasks == 0)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_EMPTY_BOARD);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Format task list: COMMAND,STATUS_OK,totalTasks,task1_id:task1_title,...
    int offset = snprintf(response, sizeof(response), "%s%s%d%s%d",
                          cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, board->totalTasks);

    for (int i = 0; i < board->totalTasks; i++)
    {
        offset += snprintf(response + offset, sizeof(response) - offset, "%s%s:%s",
                           COMMAND_SEPARATOR, board->tasks[i].taskId, board->tasks[i].taskTitle);
    }
    snprintf(response + offset, sizeof(response) - offset, "\n");

    send(ctx->clientSocket, response, strlen(response), 0);
}
