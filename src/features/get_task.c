#include "get_task.h"
#include "get_board.h" // Reuse findBoard for board lookups

/*
 * ============================================================================
 * THREAD WORKER FUNCTION FOR TASKS
 * ============================================================================
 * Each worker thread independently searches its assigned slice [startIndex, endIndex)
 * of the board's tasks array.
 */
void *searchTaskWorker(void *arg)
{
    TaskSearchData *data = (TaskSearchData *)arg;

    for (int i = data->startIndex; i < data->endIndex; i++)
    {
        int match = 0;
        /* Retained tombstone slots must never be returned as active tasks. */
        if (!data->board->tasks[i].isDeleted &&
            data->targetId != NULL && strcmp(data->board->tasks[i].taskId, data->targetId) == 0)
        {
            match = 1;
        }
        else if (!data->board->tasks[i].isDeleted &&
                 data->targetTitle != NULL && strcmp(data->board->tasks[i].taskTitle, data->targetTitle) == 0)
        {
            match = 1;
        }

        if (match)
        {
            data->foundTask = &data->board->tasks[i];
            break; // Found task in this thread's slice; stop early
        }
    }

    return NULL;
}

/*
 * ============================================================================
 * MULTI-THREADED PARALLEL SEARCH FOR TASKS
 * ============================================================================
 * Divides the board's tasks array into chunks of 100 items each and spawns
 * 1 thread per chunk (100 tasks each) to search concurrently in parallel.
 */
Task *findTaskParallel(Board *board, const char *taskId, const char *taskTitle)
{
    int total = board->totalTasks;
    int numThreads = (total + 99) / 100;

    printf("[Parallel Task Search] Launching %d worker thread(s) for %d tasks...\n", numThreads, total);

    pthread_t threads[numThreads];
    TaskSearchData searchData[numThreads];
    Task *result = NULL;

    for (int i = 0; i < numThreads; i++)
    {
        int start = i * 100;
        int end = (i + 1) * 100;
        if (end > total)
        {
            end = total;
        }

        searchData[i].board = board;
        searchData[i].targetId = taskId;
        searchData[i].targetTitle = taskTitle;
        searchData[i].startIndex = start;
        searchData[i].endIndex = end;
        searchData[i].foundTask = NULL;

        printf("  -> Task Thread %d assigned range: [%d to %d)\n", i, start, end);
        pthread_create(&threads[i], NULL, searchTaskWorker, &searchData[i]);
    }

    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
        if (searchData[i].foundTask != NULL)
        {
            result = searchData[i].foundTask;
        }
    }

    return result;
}

/*
 * ============================================================================
 * REUSABLE TASK FINDERS (BY ID & BY NAME)
 * ============================================================================
 * - If totalTasks < 100 : Single-threaded linear search loop.
 * - If totalTasks >= 100: Multi-threaded parallel search (1 thread per 100 tasks).
 */
Task *findTask(Board *board, const char *taskId)
{
    if (board == NULL || taskId == NULL) return NULL;
    int total = board->totalTasks;

    if (total < 100)
    {
        printf("[Sequential Task Search] Total tasks (%d) < 100. Using single thread loop.\n", total);
        for (int i = 0; i < total; i++)
        {
            if (!board->tasks[i].isDeleted &&
                strcmp(board->tasks[i].taskId, taskId) == 0)
            {
                return &board->tasks[i];
            }
        }
        return NULL;
    }
    else
    {
        printf("[Parallel Task Search] Total tasks (%d) >= 100. Switching to pthread multi-threading.\n", total);
        return findTaskParallel(board, taskId, NULL);
    }
}

Task *findTaskByName(Board *board, const char *taskTitle)
{
    if (board == NULL || taskTitle == NULL) return NULL;
    int total = board->totalTasks;

    if (total < 100)
    {
        printf("[Sequential Task Search] Total tasks (%d) < 100. Using single thread loop.\n", total);
        for (int i = 0; i < total; i++)
        {
            if (!board->tasks[i].isDeleted &&
                strcmp(board->tasks[i].taskTitle, taskTitle) == 0)
            {
                return &board->tasks[i];
            }
        }
        return NULL;
    }
    else
    {
        printf("[Parallel Task Search] Total tasks (%d) >= 100. Switching to pthread multi-threading.\n", total);
        return findTaskParallel(board, NULL, taskTitle);
    }
}

/*
 * ============================================================================
 * COMMAND HANDLER: GET_TASK / GET_TASKS
 * ============================================================================
 */
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

    // Reuse findBoard from get_board.h (parallel search if >= 100 boards)
    Board *board = findBoard(ctx->project, boardId);
    if (board == NULL)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Case 1: Specific Task ID requested (GET_TASK)
    if (!isEmptyOrSpace(taskId))
    {
        // Reusable parallel findTask (1 thread per 100 tasks if >= 100 tasks)
        Task *task = findTask(board, taskId);
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

    // Case 2: List all active tasks for board (GET_TASKS).
    int activeTaskCount = 0;
    for (int i = 0; i < board->totalTasks; i++)
    {
        if (!board->tasks[i].isDeleted)
            activeTaskCount++;
    }

    if (activeTaskCount == 0)
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_EMPTY_BOARD);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Format task list using active count; tombstones stay hidden from clients.
    int offset = snprintf(response, sizeof(response), "%s%s%d%s%d",
                          cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, activeTaskCount);

    for (int i = 0; i < board->totalTasks; i++)
    {
        if (board->tasks[i].isDeleted)
            continue;

        offset += snprintf(response + offset, sizeof(response) - offset, "%s%s:%s",
                           COMMAND_SEPARATOR, board->tasks[i].taskId, board->tasks[i].taskTitle);
    }
    snprintf(response + offset, sizeof(response) - offset, "\n");

    send(ctx->clientSocket, response, strlen(response), 0);
}
