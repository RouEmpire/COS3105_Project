#include "get_board.h"

/*
 * ============================================================================
 * THREAD WORKER FUNCTION
 * ============================================================================
 * Each worker thread executes this function independently in parallel.
 * It searches a specific slice of the board array from startIndex to endIndex.
 */
void *searchBoardWorker(void *arg)
{
    // Cast generic void pointer back to our typed search data structure
    BoardSearchData *data = (BoardSearchData *)arg;

    // Search only within this thread's assigned range [startIndex, endIndex)
    for (int i = data->startIndex; i < data->endIndex; i++)
    {
        if (strcmp(data->project->boards[i].boardId, data->targetId) == 0)
        {
            // Found matching board! Save it in our data structure
            data->foundBoard = &data->project->boards[i];
            break; // Stop searching early in this thread once found
        }
    }

    return NULL; // Thread finished work successfully
}

/*
 * ============================================================================
 * MULTI-THREADED PARALLEL SEARCH
 * ============================================================================
 * Divides the board array into chunks of 100 items each and spawns 1 thread
 * per chunk to search all chunks concurrently (in parallel).
 */
Board *findBoardParallel(Project *project, const char *boardId)
{
    int total = project->totalBoards;

    // Calculate how many threads are needed: 1 thread per 100 items
    // Examples: 100 boards -> 1 thread, 150 boards -> 2 threads, 250 boards -> 3 threads
    int numThreads = (total + 99) / 100;

    printf("[Parallel Search] Launching %d worker thread(s) for %d items...\n", numThreads, total);

    // Arrays to hold thread handles and data passed into each thread
    pthread_t threads[numThreads];
    BoardSearchData searchData[numThreads];
    Board *result = NULL;

    // Step 1: Create and launch all worker threads
    for (int i = 0; i < numThreads; i++)
    {
        // Define the range of items this thread is responsible for
        int start = i * 100;
        int end = (i + 1) * 100;
        if (end > total)
        {
            end = total; // Boundary check: don't search beyond total boards
        }

        // Setup the data package for thread i
        searchData[i].project = project;
        searchData[i].targetId = boardId;
        searchData[i].startIndex = start;
        searchData[i].endIndex = end;
        searchData[i].foundBoard = NULL;

        printf("  -> Thread %d assigned range: [%d to %d)\n", i, start, end);

        // pthread_create launches the thread into searchBoardWorker
        pthread_create(&threads[i], NULL, searchBoardWorker, &searchData[i]);
    }

    // Step 2: Wait for all threads to finish and collect results (pthread_join)
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL); // Wait for thread i to complete

        // Check if thread i found the target board
        if (searchData[i].foundBoard != NULL)
        {
            result = searchData[i].foundBoard;
        }
    }

    return result;
}

/*
 * ============================================================================
 * MAIN FIND BOARD SELECTOR
 * ============================================================================
 * Decides whether to use Single-Threaded or Multi-Threaded search based on
 * total item count:
 * - totalBoards < 100 : Single-threaded linear search
 * - totalBoards >= 100: Multi-threaded parallel search (pthread)
 */
Board *findBoard(Project *project, const char *boardId)
{
    int total = project->totalBoards;

    // Rule: Use simple loop if items < 100, use pthreads if items >= 100
    if (total < 100)
    {
        printf("[Sequential Search] Total items (%d) < 100. Using single thread loop.\n", total);
        
        // Simple single-threaded search loop
        for (int i = 0; i < total; i++)
        {
            if (strcmp(project->boards[i].boardId, boardId) == 0)
            {
                return &project->boards[i];
            }
        }
        return NULL;
    }
    else
    {
        printf("[Parallel Search] Total items (%d) >= 100. Switching to pthread multi-threading.\n", total);
        
        // Multi-threaded search
        return findBoardParallel(project, boardId);
    }
}

/*
 * ============================================================================
 * COMMAND HANDLER FOR TCP REQUESTS
 * ============================================================================
 * Handles GET_BOARD / GET_BOARDS commands received from netcat / TCP clients.
 */
void handleGetBoard(CommandContext *ctx)
{
    char *saveptr;
    char *boardId = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char response[MESSAGE_SIZE];
    const char *cmd = (ctx->commandName != NULL) ? ctx->commandName : COMMAND_GET_BOARD;

    // Check for invalid or empty boardId argument
    if (isEmptyOrSpace(boardId))
    {
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_INVALID_ARGUMENTS);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Find the board using our intelligent single/multi-threaded search
    Board *board = findBoard(ctx->project, boardId);

    if (board == NULL)
    {
        // Status 201: Board Not Found
        snprintf(response, sizeof(response), "%s%s%d\n", cmd, COMMAND_SEPARATOR, STATUS_BOARD_NOT_FOUND);
        send(ctx->clientSocket, response, strlen(response), 0);
        return;
    }

    // Status 0 (STATUS_OK): Send board title
    snprintf(response, sizeof(response), "%s%s%d%s%s\n", cmd, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, board->boardTitle);
    send(ctx->clientSocket, response, strlen(response), 0);
}
