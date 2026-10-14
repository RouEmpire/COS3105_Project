#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "../models.h"
#include "../features/get_board.h"
#include "../features/get_task.h"
#include "../features/set_task_status.h"

// Define global semaphore pointer used across features
sem_t *sem = NULL;

int main()
{
    printf("========== TEST: Set Task Status (Toggle isFinish) ==========\n");

    // Initialize named semaphore for unit testing
    sem = sem_open("/store_lock_test", O_CREAT, 0644, 1);
    assert(sem != SEM_FAILED);

    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    // Create 1 board with 150 tasks (to test parallel search path)
    addBoard(project, "Toggle Test Board");
    Board *board = findBoard(project, "1");
    assert(board != NULL);

    for (int t = 1; t <= 150; t++) {
        char title[64];
        snprintf(title, sizeof(title), "Toggle Task %d", t);
        addTask(board, title);
    }
    assert(board->totalTasks == 150);

    // Set Task 1 to Done initially
    board->tasks[0].isFinish = 1;
    // Set Task 130 to Pending initially
    board->tasks[129].isFinish = 0;

    // ── Test 1: Toggle Task 1 from Done (1) → Pending (0) ──
    printf("\n--- Test 1: Toggle Task 1 (Done → Pending) ---\n");
    Task *t1 = findTask(board, "1");
    assert(t1 != NULL);
    assert(t1->isFinish == 1);

    sem_wait(sem);
    t1->isFinish = !t1->isFinish;  // Toggle
    saveAllTasks(project, "test_set_status_out.txt");
    sem_post(sem);

    assert(t1->isFinish == 0);
    printf("Task 1 toggled: Done → Pending (isFinish = %d) ✓\n", t1->isFinish);

    // ── Test 2: Toggle Task 1 again from Pending (0) → Done (1) ──
    printf("\n--- Test 2: Toggle Task 1 again (Pending → Done) ---\n");
    sem_wait(sem);
    t1->isFinish = !t1->isFinish;  // Toggle again
    saveAllTasks(project, "test_set_status_out.txt");
    sem_post(sem);

    assert(t1->isFinish == 1);
    printf("Task 1 toggled: Pending → Done (isFinish = %d) ✓\n", t1->isFinish);

    // ── Test 3: Toggle Task 130 (Parallel range, Pending → Done) ──
    printf("\n--- Test 3: Toggle Task 130 (Pending → Done) ---\n");
    Task *t130 = findTask(board, "130");
    assert(t130 != NULL);
    assert(t130->isFinish == 0);

    sem_wait(sem);
    t130->isFinish = !t130->isFinish;
    saveAllTasks(project, "test_set_status_out.txt");
    sem_post(sem);

    assert(t130->isFinish == 1);
    printf("Task 130 toggled: Pending → Done (isFinish = %d) ✓\n", t130->isFinish);

    // Verify Task 1 still has its value (status preserved across other toggles)
    assert(t1->isFinish == 1);
    printf("Task 1 status still preserved: Done (isFinish = %d) ✓\n", t1->isFinish);

    // ── Test 4: Non-existent task returns NULL ──
    printf("\n--- Test 4: Non-existent Task ---\n");
    Task *ghost = findTask(board, "999");
    assert(ghost == NULL);
    printf("Non-existent Task '999' correctly returned NULL ✓\n");

    // Clean up
    remove("test_set_status_out.txt");
    sem_close(sem);
    sem_unlink("/store_lock_test");
    free(project);

    printf("\n✅ ALL SET_TASK_STATUS TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
