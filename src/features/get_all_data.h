#ifndef GET_ALL_DATA_H
#define GET_ALL_DATA_H

/*
 * ============================================================================
 * FEATURE: GET ALL DATA (Thread Pool Pattern)
 * ============================================================================
 * Educational Parallel Programming Lesson for High School / Intro CS Students:
 *
 * 1. What is a Thread Pool?
 *    Instead of spawning hundreds of new threads (which drains CPU and memory),
 *    we create a fixed team of 8 worker threads that stay alive, wait for jobs,
 *    and execute them as they arrive from a shared queue.
 *
 * 2. Why use a Synchronized Task Queue?
 *    The queue connects the PRODUCER (the parent thread reading boards.txt)
 *    to the CONSUMERS (the 8 worker threads).
 *    To prevent two threads from taking or inserting the same item at once,
 *    we protect the queue with a Mutex (Mutual Exclusion lock) and a Condition
 *    Variable (which puts worker threads to sleep when no work is available,
 *    saving CPU power, and wakes them up when a job is ready).
 *
 * 3. Shared Address Space & Mutex:
 *    Because all threads live in the same process, they share the exact same
 *    memory! To combine their loaded results into one master list without
 *    stepping on each other's toes (a Race Condition), each thread briefly
 *    locks a results mutex before appending its loaded board.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../models.h"
#include "../server.h"
#include "../protocol.h"

// Maximum number of concurrent worker threads in our pool
#define MAX_WORKER_THREADS 8

// ----------------------------------------------------------------------------
// DATA STRUCTURES
// ----------------------------------------------------------------------------

/*
 * LoadedTask: Represents one single task retrieved by a worker thread.
 */
typedef struct {
    char taskId[TASKID_SIZE];
    char taskTitle[TASK_TITLE_SIZE];
    int isFinish;
} LoadedTask;

/*
 * LoadedBoard: Represents a board with all of its associated loaded tasks.
 */
typedef struct {
    char boardId[BOARDID_SIZE];
    char boardTitle[BOARD_TITLE_SIZE];
    int taskCount;       // How many tasks were loaded for this board
    int taskCapacity;    // Current allocated capacity for the tasks array
    LoadedTask *tasks;   // Dynamically-sized array of loaded tasks
} LoadedBoard;

/*
 * SharedProjectData: The shared master data structure in memory.
 * All 8 worker threads will append their completed boards to this list.
 */
typedef struct {
    int boardCount;
    int boardCapacity;
    LoadedBoard *boards;
    pthread_mutex_t lock; // Mutex protecting the boards array from race conditions
} SharedProjectData;

/*
 * WorkItem: A job waiting in the synchronized task queue.
 * Contains the board info that a worker thread needs to search for tasks.
 */
typedef struct WorkItem {
    char boardId[BOARDID_SIZE];
    char boardTitle[BOARD_TITLE_SIZE];
    int taskLimit;         // Maximum tasks to load for this board (0 = all)
    struct WorkItem *next; // Pointer to the next job in the linked queue
} WorkItem;

/*
 * SynchronizedQueue: A thread-safe FIFO queue for our Thread Pool.
 */
typedef struct {
    WorkItem *head;                   // First item in queue
    WorkItem *tail;                   // Last item in queue
    int count;                        // Number of waiting jobs
    int isShutdown;                   // 1 when parent is done pushing all jobs
    pthread_mutex_t lock;             // Protects queue insertion and removal
    pthread_cond_t condWorkAvailable; // Signals idle workers when work is ready
} SynchronizedQueue;

/*
 * ThreadPoolContext: Bundles the queue and shared output structure so
 * each worker thread has access to both.
 */
typedef struct {
    SynchronizedQueue *queue;
    SharedProjectData *sharedData;
} ThreadPoolContext;

// ----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// ----------------------------------------------------------------------------

// High-level parallel orchestrator (creates thread pool, pushes jobs, joins threads)
SharedProjectData *fetchAllDataParallel(int boardLimit, int taskLimit);

// Clean up heap memory allocated by SharedProjectData
void freeSharedProjectData(SharedProjectData *data);

// Server command handler for GET_ALL TCP request
void handleGetAllData(CommandContext *ctx);

#endif // GET_ALL_DATA_H
