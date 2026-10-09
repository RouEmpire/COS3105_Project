#ifndef GET_BOARD_H
#define GET_BOARD_H

#include <pthread.h>
#include "../server.h"

/*
 * ============================================================================
 * FEATURE: GET_BOARD / GET_BOARDS (Parallel Search Edition)
 * ============================================================================
 * This module demonstrates Parallel Programming using POSIX Threads (pthread).
 * 
 * Rules:
 * - If total boards < 100: Use simple single-threaded loop (Sequential Search).
 * - If total boards >= 100: Use multi-threaded parallel search (1 thread per 100 items).
 *   Example: 150 items -> 2 threads (Thread 0: items 0..99, Thread 1: items 100..149).
 */

// Helper structure to pass parameters into each parallel thread
typedef struct {
    Project *project;       // Pointer to shared Project structure containing boards
    const char *targetId;   // The boardId string we are looking for
    int startIndex;         // First index in the array for this thread to check
    int endIndex;           // End index (exclusive) for this thread to stop checking
    Board *foundBoard;      // Pointer to result board if found by this thread
} BoardSearchData;

// Function Prototypes
void *searchBoardWorker(void *arg);
Board *findBoardParallel(Project *project, const char *boardId);
Board *findBoard(Project *project, const char *boardId);
void handleGetBoard(CommandContext *ctx);

#endif
