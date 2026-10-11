/*
 * ============================================================================
 * src/features/get_all_data.c
 * Feature: Get All Data (Boards and Tasks) using a Thread Pool
 * ============================================================================
 * Educational Lesson for High School Students:
 *
 * This file demonstrates the classic PRODUCER-CONSUMER Architecture using
 * a fixed THREAD POOL of 8 worker threads.
 *
 * - THE PRODUCER (Parent Thread):
 *     Reads boards from "board.txt" and puts them into a "Task Queue".
 *     Think of this like a teacher placing homework assignments into an in-box.
 *
 * - THE CONSUMERS (8 Worker Threads):
 *     Wait for assignments in the in-box, take one board at a time, find all
 *     tasks matching that board from "task.txt", and write them down.
 *
 * - SYNCHRONIZATION (Mutexes & Condition Variables):
 *     1. Queue Mutex: Prevents two workers from grabbing the same assignment.
 *     2. Condition Variable: Workers go to sleep when the in-box is empty,
 *        and wake up instantly as soon as a new assignment arrives.
 *     3. Results Mutex: Prevents workers from writing onto the same page of the
 *        shared master list at the same exact time (prevents Race Conditions).
 * ============================================================================
 */

#include "get_all_data.h"

// ----------------------------------------------------------------------------
// SECTION 1: SYNCHRONIZED TASK QUEUE IMPLEMENTATION
// ----------------------------------------------------------------------------

/*
 * queue_init:
 * Initializes a clean, empty task queue with a fresh mutex and condition variable.
 */
static void queue_init(SynchronizedQueue *q)
{
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    q->isShutdown = 0;

    // Initialize the lock that protects the queue
    pthread_mutex_init(&q->lock, NULL);

    // Initialize the condition variable used to wake sleeping workers
    pthread_cond_init(&q->condWorkAvailable, NULL);
}

/*
 * queue_push:
 * PRODUCER action: Inserts a new work item at the back (tail) of the queue.
 * Safely locks the queue, appends the item, and rings the "doorbell" (signals)
 * to wake up one waiting worker thread.
 */
static void queue_push(SynchronizedQueue *q, const char *boardId, const char *boardTitle, int taskLimit)
{
    // Step 1: Allocate a new node on the heap
    WorkItem *node = (WorkItem *)malloc(sizeof(WorkItem));
    if (node == NULL) {
        perror("Failed to allocate memory for WorkItem");
        return;
    }

    strncpy(node->boardId, boardId, sizeof(node->boardId) - 1);
    node->boardId[sizeof(node->boardId) - 1] = '\0';

    strncpy(node->boardTitle, boardTitle, sizeof(node->boardTitle) - 1);
    node->boardTitle[sizeof(node->boardTitle) - 1] = '\0';

    node->taskLimit = taskLimit;
    node->next = NULL;

    // Step 2: ACQUIRE LOCK (Enter Critical Section)
    pthread_mutex_lock(&q->lock);

    // Step 3: Insert node into the linked list queue
    if (q->tail == NULL) {
        // Queue was empty; this is both head and tail
        q->head = node;
        q->tail = node;
    } else {
        // Append to the end of the existing list
        q->tail->next = node;
        q->tail = node;
    }
    q->count++;

    // Step 4: SIGNAL! Wake up one waiting worker thread
    pthread_cond_signal(&q->condWorkAvailable);

    // Step 5: RELEASE LOCK (Exit Critical Section)
    pthread_mutex_unlock(&q->lock);
}

/*
 * queue_pop:
 * CONSUMER action: Called by worker threads to retrieve the next available job.
 * If the queue is empty, the worker sleeps efficiently via pthread_cond_wait().
 * Returns 1 if a job was retrieved, or 0 if the queue is shut down and empty.
 */
static int queue_pop(SynchronizedQueue *q, WorkItem *outItem)
{
    // Step 1: ACQUIRE LOCK before inspecting queue
    pthread_mutex_lock(&q->lock);

    // Step 2: SLEEP WHILE EMPTY
    // We use a while loop because a thread might wake up spuriously (false alarm),
    // so it must re-verify that an item is actually available.
    while (q->head == NULL && !q->isShutdown) {
        // pthread_cond_wait atomically unlocks the mutex and puts this thread
        // to sleep. When signaled, it wakes up and re-locks the mutex!
        pthread_cond_wait(&q->condWorkAvailable, &q->lock);
    }

    // Step 3: Check if we woke up because the system is shutting down
    if (q->head == NULL && q->isShutdown) {
        pthread_mutex_unlock(&q->lock);
        return 0; // No more work will ever arrive; worker should exit!
    }

    // Step 4: Dequeue the front (head) node
    WorkItem *node = q->head;
    *outItem = *node; // Copy data over to caller's struct
    q->head = node->next;

    if (q->head == NULL) {
        q->tail = NULL; // Queue is now empty
    }
    q->count--;

    // Free the temporary queue node
    free(node);

    // Step 5: RELEASE LOCK
    pthread_mutex_unlock(&q->lock);
    return 1; // Successfully grabbed a work item!
}

/*
 * queue_shutdown:
 * Signals to all worker threads that no more boards will be added.
 * Broadcasts to wake up ALL sleeping threads so they can gracefully terminate.
 */
static void queue_shutdown(SynchronizedQueue *q)
{
    pthread_mutex_lock(&q->lock);
    q->isShutdown = 1;
    // Wake up EVERY sleeping worker thread so they see isShutdown == 1 and exit
    pthread_cond_broadcast(&q->condWorkAvailable);
    pthread_mutex_unlock(&q->lock);
}

/*
 * queue_destroy:
 * Cleans up mutex and condition variable resources.
 */
static void queue_destroy(SynchronizedQueue *q)
{
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->condWorkAvailable);
}

// ----------------------------------------------------------------------------
// SECTION 2: WORKER THREAD LOGIC (PULL & PROCESS)
// ----------------------------------------------------------------------------

/*
 * workerThread:
 * The function executed by all 8 worker threads in parallel.
 * Each worker repeatedly:
 * 1. Pops a board from the synchronized queue.
 * 2. Reads "task.txt" to collect tasks belonging to that board.
 * 3. Safely locks the shared results structure and appends its completed board.
 */
static void *workerThread(void *arg)
{
    ThreadPoolContext *ctx = (ThreadPoolContext *)arg;
    WorkItem item;

    // Loop continuously as long as there are jobs to pop
    while (queue_pop(ctx->queue, &item))
    {
        // --------------------------------------------------------------------
        // Step A: Prepare local board container
        // --------------------------------------------------------------------
        LoadedBoard board;
        strncpy(board.boardId, item.boardId, sizeof(board.boardId) - 1);
        board.boardId[sizeof(board.boardId) - 1] = '\0';

        strncpy(board.boardTitle, item.boardTitle, sizeof(board.boardTitle) - 1);
        board.boardTitle[sizeof(board.boardTitle) - 1] = '\0';

        board.taskCount = 0;
        // Allocate initial memory capacity for tasks
        board.taskCapacity = (item.taskLimit > 0 && item.taskLimit < 64) ? item.taskLimit : 64;
        board.tasks = (LoadedTask *)malloc(board.taskCapacity * sizeof(LoadedTask));

        // --------------------------------------------------------------------
        // Step B: Read tasks for this board from task.txt
        // --------------------------------------------------------------------
        // Multiple threads can safely open and read "task.txt" simultaneously
        // because read-only file access does not conflict!
        FILE *taskFile = fopen(TASK_FILENAME, "r");
        if (taskFile != NULL)
        {
            char lineBuffer[256];
            size_t idLen = strlen(item.boardId);

            while (fgets(lineBuffer, sizeof(lineBuffer), taskFile) != NULL)
            {
                // Quick prefix match: does line start with "<boardId>,"?
                if (strncmp(lineBuffer, item.boardId, idLen) == 0 && lineBuffer[idLen] == ',')
                {
                    // Tokenize format: boardId,taskId,taskTitle,isFinish
                    char *saveptr;
                    char *t_boardId = strtok_r(lineBuffer, ",\r\n", &saveptr);
                    char *t_taskId = strtok_r(NULL, ",\r\n", &saveptr);
                    char *t_taskTitle = strtok_r(NULL, ",\r\n", &saveptr);
                    char *t_isFinish = strtok_r(NULL, ",\r\n", &saveptr);

                    if (t_taskId && t_taskTitle && t_isFinish)
                    {
                        // Expand array dynamically if needed
                        if (board.taskCount >= board.taskCapacity) {
                            board.taskCapacity *= 2;
                            board.tasks = (LoadedTask *)realloc(board.tasks, board.taskCapacity * sizeof(LoadedTask));
                        }

                        LoadedTask *t = &board.tasks[board.taskCount++];
                        strncpy(t->taskId, t_taskId, sizeof(t->taskId) - 1);
                        t->taskId[sizeof(t->taskId) - 1] = '\0';

                        strncpy(t->taskTitle, t_taskTitle, sizeof(t->taskTitle) - 1);
                        t->taskTitle[sizeof(t->taskTitle) - 1] = '\0';

                        t->isFinish = atoi(t_isFinish);

                        // Check if we hit the user's task limit for this board
                        if (item.taskLimit > 0 && board.taskCount >= item.taskLimit) {
                            break; // Reached limit, stop scanning task file for this board
                        }
                    }
                }
            }
            fclose(taskFile);
        }

        // --------------------------------------------------------------------
        // Step C: Append to the Shared Data Structure (CRITICAL SECTION)
        // --------------------------------------------------------------------
        // Because 8 threads run simultaneously, we MUST use a mutex lock here!
        // Without this lock, two threads could write to the same index and
        // corrupt the board array (Race Condition).
        pthread_mutex_lock(&ctx->sharedData->lock);

        if (ctx->sharedData->boardCount >= ctx->sharedData->boardCapacity) {
            ctx->sharedData->boardCapacity *= 2;
            ctx->sharedData->boards = (LoadedBoard *)realloc(
                ctx->sharedData->boards,
                ctx->sharedData->boardCapacity * sizeof(LoadedBoard)
            );
        }

        ctx->sharedData->boards[ctx->sharedData->boardCount++] = board;

        pthread_mutex_unlock(&ctx->sharedData->lock);
    }

    return NULL; // Worker finishes peacefully
}

// ----------------------------------------------------------------------------
// SECTION 3: SORTING COMPARATOR (CLEAN NUMERICAL ORDER)
// ----------------------------------------------------------------------------

/*
 * compareBoards:
 * Because worker threads run at varying speeds in parallel, boards might be
 * appended out of numerical order. This comparator allows qsort to arrange them
 * cleanly from lowest Board ID to highest Board ID.
 */
static int compareBoards(const void *a, const void *b)
{
    const LoadedBoard *ba = (const LoadedBoard *)a;
    const LoadedBoard *bb = (const LoadedBoard *)b;
    int id_a = atoi(ba->boardId);
    int id_b = atoi(bb->boardId);
    if (id_a != id_b) {
        return id_a - id_b;
    }
    return strcmp(ba->boardId, bb->boardId);
}

// ----------------------------------------------------------------------------
// SECTION 4: HIGH-LEVEL ORCHESTRATOR (THREAD POOL LIFECYCLE)
// ----------------------------------------------------------------------------

/*
 * fetchAllDataParallel:
 * The master function implementing the Thread Pool pattern:
 * 1. Sets up the Synchronized Task Queue and Shared Results structure.
 * 2. Launches a pool of 8 worker threads.
 * 3. Parent thread reads "board.txt" and pushes Board IDs into the queue.
 * 4. Shuts down the queue and joins all 8 worker threads.
 * 5. Sorts the results and returns the master structure.
 */
SharedProjectData *fetchAllDataParallel(int boardLimit, int taskLimit)
{
    printf("[Thread Pool] Initializing Thread Pool (max %d workers)...\n", MAX_WORKER_THREADS);

    // 1. Initialize the Task Queue
    SynchronizedQueue queue;
    queue_init(&queue);

    // 2. Initialize the Shared Data Structure
    SharedProjectData *sharedData = (SharedProjectData *)malloc(sizeof(SharedProjectData));
    sharedData->boardCount = 0;
    sharedData->boardCapacity = (boardLimit > 0 && boardLimit < 128) ? boardLimit : 128;
    sharedData->boards = (LoadedBoard *)malloc(sharedData->boardCapacity * sizeof(LoadedBoard));
    pthread_mutex_init(&sharedData->lock, NULL);

    // Package the context shared by the worker pool
    ThreadPoolContext poolContext = {
        .queue = &queue,
        .sharedData = sharedData
    };

    // 3. Launch the 8 Worker Threads
    pthread_t workers[MAX_WORKER_THREADS];
    for (int i = 0; i < MAX_WORKER_THREADS; i++) {
        if (pthread_create(&workers[i], NULL, workerThread, &poolContext) != 0) {
            perror("Failed to create worker thread");
        }
    }
    printf("[Thread Pool] All %d worker threads launched and listening.\n", MAX_WORKER_THREADS);

    // 4. PARENT PROCESS: Read board.txt and push Board IDs into queue
    FILE *boardFile = fopen(BOARD_FILENAME, "r");
    int boardsPushed = 0;

    if (boardFile != NULL)
    {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), boardFile) != NULL)
        {
            buffer[strcspn(buffer, "\r\n")] = '\0';
            if (isEmptyOrSpace(buffer)) continue;

            char *saveptr;
            char *bId = strtok_r(buffer, ",", &saveptr);
            char *bTitle = strtok_r(NULL, ",", &saveptr);

            if (bId != NULL && bTitle != NULL && !isEmptyOrSpace(bId) && !isEmptyOrSpace(bTitle))
            {
                queue_push(&queue, bId, bTitle, taskLimit);
                boardsPushed++;

                // Stop if we reached user's requested board limit
                if (boardLimit > 0 && boardsPushed >= boardLimit) {
                    printf("[Thread Pool] Reached requested board limit of %d boards.\n", boardLimit);
                    break;
                }
            }
        }
        fclose(boardFile);
    }
    else
    {
        printf("[Thread Pool] Warning: Could not open %s\n", BOARD_FILENAME);
    }

    printf("[Thread Pool] Parent enqueued %d boards. Notifying workers of completion...\n", boardsPushed);

    // 5. Signal queue shutdown (broadcasts to all workers)
    queue_shutdown(&queue);

    // 6. Join all worker threads (wait for them to finish all jobs)
    for (int i = 0; i < MAX_WORKER_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }
    printf("[Thread Pool] All %d worker threads completed their work.\n", MAX_WORKER_THREADS);

    // Clean up queue synchronization resources
    queue_destroy(&queue);

    // 7. Sort boards in clean numerical order
    if (sharedData->boardCount > 1) {
        qsort(sharedData->boards, sharedData->boardCount, sizeof(LoadedBoard), compareBoards);
    }

    return sharedData;
}

/*
 * freeSharedProjectData:
 * Frees all allocated memory in the shared structure to prevent leaks.
 */
void freeSharedProjectData(SharedProjectData *data)
{
    if (data == NULL) return;

    for (int i = 0; i < data->boardCount; i++) {
        if (data->boards[i].tasks != NULL) {
            free(data->boards[i].tasks);
        }
    }
    if (data->boards != NULL) {
        free(data->boards);
    }
    pthread_mutex_destroy(&data->lock);
    free(data);
}

// ----------------------------------------------------------------------------
// SECTION 5: COMMAND HANDLER FOR TCP REQUESTS
// ----------------------------------------------------------------------------

/*
 * handleGetAllData:
 * Handles incoming TCP requests: "GET_ALL,<board_limit>,<task_limit>"
 * Streams output lines over the client socket:
 * Header: GET_ALL,0,<boardCount>
 * For each board: BOARD,<boardId>,<boardTitle>,<taskCount>
 * For each task:  TASK,<taskId>,<taskTitle>,<isFinish>
 * Trailer: END_ALL
 */
void handleGetAllData(CommandContext *ctx)
{
    char *saveptr;
    char *boardLimitStr = strtok_r(ctx->rawInput, ",\n\r", &saveptr);
    char *taskLimitStr = strtok_r(NULL, ",\n\r", &saveptr);

    int boardLimit = 0;
    int taskLimit = 0;

    if (boardLimitStr != NULL && isNumeric(boardLimitStr)) {
        boardLimit = atoi(boardLimitStr);
    }
    if (taskLimitStr != NULL && isNumeric(taskLimitStr)) {
        taskLimit = atoi(taskLimitStr);
    }

    printf("[GET_ALL] Client requested data (Board Limit: %d, Task Limit: %d)\n",
           boardLimit, taskLimit);

    // Execute Thread Pool fetch
    SharedProjectData *data = fetchAllDataParallel(boardLimit, taskLimit);

    if (data == NULL || data->boardCount == 0)
    {
        char emptyResp[128];
        snprintf(emptyResp, sizeof(emptyResp), "%s%s%d%s0\nEND_ALL\n",
                 COMMAND_GET_ALL, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR);
        send(ctx->clientSocket, emptyResp, strlen(emptyResp), 0);
        if (data) freeSharedProjectData(data);
        return;
    }

    // Stream results line by line over TCP socket
    char lineBuf[512];

    // Header line: GET_ALL,0,<boardCount>
    snprintf(lineBuf, sizeof(lineBuf), "%s%s%d%s%d\n",
             COMMAND_GET_ALL, COMMAND_SEPARATOR, STATUS_OK, COMMAND_SEPARATOR, data->boardCount);
    send(ctx->clientSocket, lineBuf, strlen(lineBuf), 0);

    for (int b = 0; b < data->boardCount; b++)
    {
        LoadedBoard *board = &data->boards[b];

        // Board line: BOARD,<boardId>,<boardTitle>,<taskCount>
        snprintf(lineBuf, sizeof(lineBuf), "BOARD,%s,%s,%d\n",
                 board->boardId, board->boardTitle, board->taskCount);
        send(ctx->clientSocket, lineBuf, strlen(lineBuf), 0);

        // Task lines: TASK,<taskId>,<taskTitle>,<isFinish>
        for (int t = 0; t < board->taskCount; t++)
        {
            LoadedTask *task = &board->tasks[t];
            snprintf(lineBuf, sizeof(lineBuf), "TASK,%s,%s,%d\n",
                     task->taskId, task->taskTitle, task->isFinish);
            send(ctx->clientSocket, lineBuf, strlen(lineBuf), 0);
        }
    }

    // End marker line: END_ALL
    snprintf(lineBuf, sizeof(lineBuf), "END_ALL\n");
    send(ctx->clientSocket, lineBuf, strlen(lineBuf), 0);

    // Clean up memory
    freeSharedProjectData(data);
}
