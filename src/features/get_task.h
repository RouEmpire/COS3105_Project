#ifndef GET_TASK_H
#define GET_TASK_H

#include <pthread.h>
#include "../server.h"

/*
 * ============================================================================
 * FEATURE: GET_TASK / GET_TASKS & PARALLEL TASK SEARCH
 * ============================================================================
 * Reusable Multi-Threaded Task Search (Matching get_board.c design):
 * - If total tasks < 100: Use simple single-threaded loop (Sequential Search).
 * - If total tasks >= 100: Use multi-threaded parallel search (1 thread per 100 items).
 *   Example: 250 items -> 3 threads (Thread 0: 0..99, Thread 1: 100..199, Thread 2: 200..249).
 */

// Helper structure to pass parameters into each parallel task search thread
typedef struct {
    Board *board;            // Board containing the tasks array
    const char *targetId;    // Task ID to match (NULL if searching by title)
    const char *targetTitle; // Task Title to match (NULL if searching by ID)
    int startIndex;          // First index in this thread's assigned range
    int endIndex;            // End index (exclusive) for this thread
    Task *foundTask;         // Pointer to matching Task if found by this thread
} TaskSearchData;

// Function Prototypes for Reusable Parallel Task Searching
void *searchTaskWorker(void *arg);
Task *findTaskParallel(Board *board, const char *taskId, const char *taskTitle);
Task *findTask(Board *board, const char *taskId);
Task *findTaskByName(Board *board, const char *taskTitle);

void handleGetTask(CommandContext *ctx);

#endif // GET_TASK_H
